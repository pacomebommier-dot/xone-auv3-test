#import "XoneMidiAudioUnit.h"
#import <AVFoundation/AVFoundation.h>
#import <string.h>
#include "../Shared/XoneHostEngine.hpp"
#include "../Shared/XoneHostGrid.hpp"
#include "../Shared/XoneSh01aProfile.hpp"
#include "../Shared/XoneValidationFixture.hpp"
#include "../Shared/XoneHostScheduler.hpp"
#include <cmath>

namespace {
NSString *const SharedShBaseKey=@"XONE_SHARED_SH01A_BASE_V1";
constexpr uint32_t ShDestination=0x53483031u;
NSData *encodeSharedShBase(xone::host::ShSourceBaseImage image) {
    if(!image.base.valid||image.destination!=ShDestination)return nil;
    xone::host::DocumentBytes bytes;
    xone::host::DocumentWire wire(bytes);
    if(!xone::host::documentShBase(wire,image))return nil;
    return [NSData dataWithBytes:bytes.data() length:bytes.size()];
}
bool decodeSharedShBase(NSData *bytes,xone::host::ShSourceBaseImage& image) {
    if(![bytes isKindOfClass:[NSData class]])return false;
    xone::host::DocumentWire wire(static_cast<const uint8_t *>(bytes.bytes),bytes.length);
    return xone::host::documentShBase(wire,image)&&wire.complete()&&image.base.valid&&image.destination==ShDestination;
}
}

@implementation XoneMidiAudioUnit {
    AUAudioUnitBusArray *_inputBusArray;
    AUAudioUnitBusArray *_outputBusArray;
    AUParameterTree *_nativeParameterTree;
    // Ãƒâ€°tabli avant de dÃƒÂ©marrer la boucle de rendu ; ne pas muter en cours de rendu.
    AUMIDIOutputEventBlock _cachedMIDIOutput;
    AUHostTransportStateBlock _cachedTransport;
    AUHostMusicalContextBlock _cachedMusicalContext;
    xone::host::HostGrid _musicalGrid;
    xone::host::HostEngine _engine;
    xone::host::ResetDefaults _resetDefaultsForHost; // host thread, protected by @synchronized
    NSData *_loadedSharedShBase;
    bool _transportKnown, _moving;
    double _gestureElapsedMs, _gestureSampleRate;
}

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
                                     options:(AudioComponentInstantiationOptions)options
                                       error:(NSError * _Nullable __autoreleasing *)outError {
    self = [super initWithComponentDescription:componentDescription options:options error:outError];
    if (!self) return nil;
#if TARGET_OS_IOS
    if (!_engine.initializeSynthDestinations()) return nil;
    xone::host::ShSourceBaseImage shared{};
    NSData *saved=[[NSUserDefaults standardUserDefaults] dataForKey:SharedShBaseKey];
    if(decodeSharedShBase(saved,shared)){
        _engine.loadSharedShBaseBeforeRender(shared);
        _loadedSharedShBase=saved;
    }
#endif

    // Busses requis par le contrat AUAudioUnit, mÃƒÂªme pour un processeur MIDI pur.
    AVAudioFormat *format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100 channels:2];
    AUAudioUnitBus *inputBus = [[AUAudioUnitBus alloc] initWithFormat:format error:outError];
    AUAudioUnitBus *outputBus = [[AUAudioUnitBus alloc] initWithFormat:format error:outError];
    if (!inputBus || !outputBus) return nil;
    _inputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                            busType:AUAudioUnitBusTypeInput
                                                             busses:@[inputBus]];
    _outputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                             busType:AUAudioUnitBusTypeOutput
                                                              busses:@[outputBus]];
    AUParameter *presetFace=[AUParameterTree createParameterWithIdentifier:@"presetEditor"
        name:@"PRESET editor" address:0 min:0 max:1 unit:kAudioUnitParameterUnit_Boolean
        unitName:nil flags:kAudioUnitParameterFlag_IsReadable|kAudioUnitParameterFlag_IsWritable
        valueStrings:nil dependentParameters:nil];
    AUParameter *refresh=[AUParameterTree createParameterWithIdentifier:@"refreshControllerLEDs"
        name:@"Refresh controller LEDs" address:1 min:0 max:1 unit:kAudioUnitParameterUnit_Boolean
        unitName:nil flags:kAudioUnitParameterFlag_IsReadable|kAudioUnitParameterFlag_IsWritable
        valueStrings:nil dependentParameters:nil];
    _nativeParameterTree=[AUParameterTree createTreeWithChildren:@[presetFace,refresh]];
    __weak XoneMidiAudioUnit *weakUnit=self;
    _nativeParameterTree.implementorValueObserver=^(AUParameter *parameter,AUValue value){
        XoneMidiAudioUnit *unit=weakUnit;
        if(!unit||!std::isfinite(value))return;
        if(parameter.address==0)unit->_engine.requestPresetPage(value>=0.5f);
        else if(parameter.address==1&&value>=0.5f)unit->_engine.requestLedRefresh();
    };
    _nativeParameterTree.implementorValueProvider=^AUValue(AUParameter *parameter){
        XoneMidiAudioUnit *unit=weakUnit;
        return unit&&parameter.address==0&&unit->_engine.presetPageVisibleForHost()?1.0f:0.0f;
    };
    return self;
}

- (AUParameterTree *)parameterTree { return _nativeParameterTree; }
- (AUAudioUnitBusArray *)inputBusses { return _inputBusArray; }
- (AUAudioUnitBusArray *)outputBusses { return _outputBusArray; }
- (NSArray<NSString *> *)MIDIOutputNames { return @[@"XONE V0.2 TEST OUT", @"SYNTH 1", @"SYNTH 2", @"SYNTH 3", @"SYNTH 4", @"SYNTH 5", @"SYNTH 6"]; }

- (BOOL)allocateRenderResourcesAndReturnError:(NSError * _Nullable __autoreleasing *)outError {
    if (![super allocateRenderResourcesAndReturnError:outError]) return NO;
    _engine.enableNativeMutePage(true);
    _cachedMIDIOutput = [self.MIDIOutputEventBlock copy];
    _cachedTransport = [self.transportStateBlock copy];
    _cachedMusicalContext = [self.musicalContextBlock copy];
    _musicalGrid.reset();
    _transportKnown = false;
    _gestureSampleRate = self.outputBusses[0].format.sampleRate;
    return YES;
}

- (void)deallocateRenderResources {
    // Host has stopped rendering. Cancel transient work, preserve real OFF debts.
    // No MIDI callback outside render: reallocation retries at its first buffer.
    _engine.stop(0);
    _transportKnown = false;
    _cachedMIDIOutput = nil;
    _cachedTransport = nil;
    _cachedMusicalContext = nil;
    _musicalGrid.reset();
    [super deallocateRenderResources];
}

// Host state methods stay off render. Apple's default fullStateForDocument
// forwards to fullState, so both host save APIs share this single implementation.
- (NSDictionary<NSString *,id> *)fullState {
    @synchronized (self) {
        NSMutableDictionary *state = [[super fullState] mutableCopy];
        if (!state) state = [NSMutableDictionary dictionary];
        [state removeObjectForKey:@"XONE_LAST_LIVE_V1"];
        [state removeObjectForKey:@"XONE_DOCUMENT_V2"];
        [state removeObjectForKey:@"XONE_SH01A_MIRRORS_V1"];
        const auto sh=_engine.sh01aForHost();
        if(sh.enabled){
            xone::host::Sh01aAttachmentBytes hardware{};
            if(xone::host::encodeSh01aAttachment(sh,hardware))
                state[@"XONE_SH01A_MIRRORS_V1"]=[NSData dataWithBytes:hardware.data() length:hardware.size()];
        }
        [state removeObjectForKey:@"XONE_RESET_DEFAULTS_V1"];
        xone::host::ResetDefaultsBytes defaults{};
        if(xone::host::encodeResetDefaults(_resetDefaultsForHost,defaults))
            state[@"XONE_RESET_DEFAULTS_V1"]=[NSData dataWithBytes:defaults.data() length:defaults.size()];
        xone::host::SessionBytes bytes{};
        const auto image = _engine.sessionForHost();
        if (xone::host::encodeSession(image, bytes))
            state[@"XONE_LAST_LIVE_V1"] = [NSData dataWithBytes:bytes.data() length:bytes.size()];
        xone::host::DocumentBytes document;
        const auto imageForDocument=_engine.documentForHost();
        if(imageForDocument.sixSynths){
            NSData *updated=encodeSharedShBase(imageForDocument.shBase);
            if(updated&&![updated isEqualToData:_loadedSharedShBase]){
                [[NSUserDefaults standardUserDefaults] setObject:updated forKey:SharedShBaseKey];
                _loadedSharedShBase=updated;
            }
        }
        if(xone::host::encodeDocument(imageForDocument,document))
            state[@"XONE_DOCUMENT_V2"]=[NSData dataWithBytes:document.data() length:document.size()];
        return state;
    }
}
- (void)setFullState:(NSDictionary<NSString *,id> *)state {
    @synchronized (self) {
        xone::host::Sh01aAttachment sh;
        id shObject=state[@"XONE_SH01A_MIRRORS_V1"];
        if(shObject){
            if(![shObject isKindOfClass:[NSData class]])return;
            NSData *data=(NSData *)shObject;
            if(!xone::host::decodeSh01aAttachment((const uint8_t *)data.bytes,data.length,sh))return;
        }
        // Optional independent configuration; absent in old documents = UNKNOWN defaults.
        // Decode it before admitting any musical restore. No defaults are applied here.
        xone::host::ResetDefaults defaults;
        id defaultsObject=state[@"XONE_RESET_DEFAULTS_V1"];
        if(defaultsObject){
            if(![defaultsObject isKindOfClass:[NSData class]])return;
            NSData *data=(NSData *)defaultsObject;
            if(!xone::host::decodeResetDefaults((const uint8_t *)data.bytes,data.length,defaults))return;
        }
        const auto configureDefaults=[&](bool followRestore){
            _engine.requestResetDefaults(defaults,followRestore);_resetDefaultsForHost=defaults;
            _engine.requestSh01aAttachment(sh,followRestore);
        };
        id document=state[@"XONE_DOCUMENT_V2"];
        if(document){
            if(![document isKindOfClass:[NSData class]])return;
            NSData *data=(NSData *)document;xone::host::DocumentImage image{};
            if(!xone::host::decodeDocument((const uint8_t *)data.bytes,data.length,image)||
               (sh.enabled&&sh.config!=image.live.config))return;
            if(image.sixSynths){
                xone::host::ShSourceBaseImage shared{};
                NSData *saved=[[NSUserDefaults standardUserDefaults] dataForKey:SharedShBaseKey];
                if(decodeSharedShBase(saved,shared)&&shared.base.config==image.live.config)
                    image.shBase=shared;
            }
            if(!_engine.requestDocumentRestore(image))return;
            _loadedSharedShBase=image.sixSynths?encodeSharedShBase(image.shBase):nil;
            configureDefaults(true);[super setFullState:state];return;
        }
        bool restored=false;
        id payload = state[@"XONE_LAST_LIVE_V1"];
        if (payload) {
            if (![payload isKindOfClass:[NSData class]]) return;
            NSData *data = (NSData *)payload;
            xone::host::SessionImage image{};
            if (!xone::host::decodeSession((const uint8_t *)data.bytes, data.length, image)) return;
            if ((sh.enabled&&sh.config!=image.config)||!_engine.requestSessionRestore(image)) return;
            restored=true;
        }
        if(sh.enabled&&!restored&&sh.config!=_engine.sessionForHost().config)return;
        configureDefaults(restored);[super setFullState:state];
    }
}

- (BOOL)configureSh01aSource:(NSInteger)source channel:(NSInteger)channel {
    @synchronized (self) {
        if(source<0||source>3||channel<1||channel>16||(channel>=7&&channel<=15))return NO;
        const auto old=_engine.sh01aForHost();const auto document=_engine.documentForHost();
        // Reconfiguration must never silently assign another route to existing mirrors.
        if(document.sixSynths&&(source>=2||document.current.alternatives[static_cast<unsigned>(source)]!=0))return NO;
        if(old.enabled)return (document.sixSynths||old.source==source)&&old.image.profile.channel==channel-1&&
            old.config==document.live.config&&old.alternative==document.current.alternatives[static_cast<unsigned>(source)];
        const auto attachment=xone::host::provisionalSh01aAttachment(static_cast<uint8_t>(source),
            document.live.config,document.current.alternatives[static_cast<unsigned>(source)],
            ShDestination,static_cast<uint8_t>(channel-1),0,1000);
        return _engine.requestSh01aAttachment(attachment);
    }
}

- (BOOL)loadAumValidationFixture {
    @synchronized (self) {
        return _engine.requestSessionRestore(xone::host::aumValidationFixture());
    }
}

- (AUInternalRenderBlock)internalRenderBlock {
    // Capturer seulement l'adresse du slot de sortie, pas d'envoi de message ÃƒÂ  self
    // dans le rendu temps rÃƒÂ©el. Ne pas allouer ni journaliser dans ce bloc.
    AUMIDIOutputEventBlock __strong *slot = &_cachedMIDIOutput;
    auto *engine = &_engine;
    auto *known = &_transportKnown;
    auto *moving = &_moving;
    auto *elapsedMs = &_gestureElapsedMs;
    const double *sampleRate = &_gestureSampleRate;
    AUHostTransportStateBlock __strong *transportSlot = &_cachedTransport;
    AUHostMusicalContextBlock __strong *musicSlot = &_cachedMusicalContext;
    auto *grid = &_musicalGrid;
    return ^AUAudioUnitStatus(AudioUnitRenderActionFlags *actionFlags,
                             const AudioTimeStamp *timestamp,
                             AVAudioFrameCount frameCount,
                             NSInteger outputBusNumber,
                             AudioBufferList *outputData,
                             const AURenderEvent *realtimeEventListHead,
                             AURenderPullInputBlock pullInputBlock) {
        (void)actionFlags; (void)outputBusNumber; (void)pullInputBlock;
        engine->beginRenderBuffer();
        // Read the rate initialized at allocation, even if this block was fetched earlier.
        const double rate = *sampleRate > 0 ? *sampleRate : 44100;
        AUMIDIOutputEventBlock __unsafe_unretained sendMIDI = *slot;
        AUHostTransportStateBlock __unsafe_unretained transport = *transportSlot;
        AUHostMusicalContextBlock __unsafe_unretained music = *musicSlot;
        const auto sendNominal = [&](AUEventSampleTime sample,int api,int type,const uint8_t* bytes) {
            const uint8_t cable=engine->midiOutputCable(api,type);
            const int address=engine->midiOutputAddress(api,type);
            if(address==api)return sendMIDI(sample,cable,3,bytes);
            const uint8_t mapped[]{static_cast<uint8_t>((bytes[0]&0xF0)+6+address/128),
                                   static_cast<uint8_t>(address%128),bytes[2]};
            return sendMIDI(sample,cable,3,mapped);
        };
        const auto flushStop = [&]() {
            for (unsigned n=0; n<702; ++n) {
                const auto packet=engine->nextStopPacket();
                if (!packet.emit) break;
                if (!engine->canSend(packet)) continue;
                if (sendMIDI) engine->sent(packet,sendNominal(packet.sample,packet.api,packet.type,packet.bytes));
                else engine->noOutput(packet);
            }
        };
        const auto bufferStart = static_cast<AUEventSampleTime>(timestamp->mSampleTime);
        // MUTE is the fixed virtual NOTE528..543 S7 API. A failed callback
        // leaves the target dirty; next render retries without claiming delivery.
        const auto flushMute = [&](AUEventSampleTime sample) {
            unsigned cursor=0;
            for(unsigned n=0;n<16;++n){
                const auto packet=engine->nextMutePacket(cursor);
                if(!packet.valid)break;
                if(sendMIDI){
                    const OSStatus result=sendMIDI(sample,0,3,packet.bytes);
                    engine->ackMutePacket(packet.target,result==noErr);
                }
            }
        };
        // SOFT choices are applied at the host BAR. Callback failure keeps
        // that logical NOTE dirty; retry is bounded once per render pass.
        const auto flushSoft = [&](AUEventSampleTime sample) {
            if(!sendMIDI)return;
            unsigned cursor=0;
            for(unsigned n=0;n<128;++n){
                const auto packet=engine->nextSoftPacket(cursor);
                if(!packet.valid)break;
                const bool ok=sendMIDI(sample,0,3,packet.bytes)==noErr;
                engine->ackSoftPacket(packet,ok);
                // Success can unlock same-page OFF -> ON at this sample.
                // Failure must not spin and must block the dependent ON.
                cursor=ok?0:packet.index+1;
            }
        };
        // S7 TOP ACTION impulses are FIFO and never a persistent NOTE state.
        // Failed ON is conservatively followed by OFF; failed OFF is retried.
        // Source CUE/LOAD CC931..946, emitted only after actual host BAR.
        // Callback success = MIDI delivery, never synth PATCH/BASE ACK.
        const auto flushCueLoad = [&](AUEventSampleTime sample) {
            if(!sendMIDI)return;
            for(unsigned n=0;n<16;++n){
                const auto packet=engine->nextCueLoadPacket();
                if(!packet.valid)break;
                const bool ok=sendMIDI(sample,0,3,packet.bytes.data())==noErr;
                engine->ackCueLoadPacket(packet,ok,sample);
            }
        };
        const auto flushSource = [&](AUEventSampleTime sample) {
            if(!sendMIDI)return;
            for(unsigned n=0;n<2048;++n){
                const auto packet=engine->nextSourcePacket();if(!packet.valid)break;
                const bool ok=sendNominal(sample,packet.api,packet.type,packet.bytes.data())==noErr;
                engine->ackSourcePacket(packet,ok,sample);if(!ok)break;
            }
        };
        const auto flushSh01a = [&](AUEventSampleTime sample,int64_t ms) {
            if(!sendMIDI)return;
            for(unsigned n=0;n<37;++n){
                const auto packet=engine->nextSh01aPacket();if(!packet.valid)break;
                const bool ok=sendMIDI(sample,engine->sh01aOutputCable(),static_cast<NSInteger>(packet.length),packet.bytes.data())==noErr;
                engine->ackSh01aPacket(packet,ok,sample,ms);if(!ok)break;
            }
        };
        bool failedTopOffInThisBuffer=false;
        const auto flushTop = [&](AUEventSampleTime sample) {
            // Multiple flush sites (MIDI edge + end-of-buffer timer) share
            // one render pass. A failed OFF must NOT be retried again inside
            // the same buffer; its next chance is the next render buffer.
            if(!sendMIDI||failedTopOffInThisBuffer)return;
            for(unsigned n=0;n<64;++n) {
                const auto packet=engine->nextTopPulse();
                if(!packet.valid)break;
                const bool success=sendMIDI(sample,0,3,packet.bytes)==noErr;
                engine->ackTopPulse(packet,success,sample);
                // Never overtake a failed OFF with the next queued TOP.
                if(!success&&!packet.on){failedTopOffInThisBuffer=true;break;}
            }
        };
        const auto flushMuteLeds = [&](AUEventSampleTime sample,int64_t ms) {
            engine->prepareMuteLeds(ms);
            if(!sendMIDI)return;
            unsigned cursor=0;
            for(unsigned n=0;n<136;++n){
                const auto packet=engine->nextMuteLedPacket(cursor);
                if(!packet.valid)break;
                const bool success=sendMIDI(sample,0,3,packet.bytes)==noErr;
                engine->ackMuteLedPacket(packet,success);
                // A failed target retries next render; do not spin or block peers.
                if(!success)cursor=packet.target+1;
            }
        };
        flushMute(bufferStart); // old failed writes, no fabricated BAR
        flushSoft(bufferStart); // retry only previously applied SOFT notes
        engine->beginCueLoadRetryPass();
        flushCueLoad(bufferStart); // retry only a previously committed BAR action
        // Retry only genuine outstanding STOP OFF debts. This precedes any
        // fresh physical MIDI in this buffer and cannot invent a note ON.
        engine->beginStopRetryPass(bufferStart);
        flushStop();
        if (transport) {
            AUHostTransportStateFlags flags = static_cast<AUHostTransportStateFlags>(0);
            if (transport(&flags, nullptr, nullptr, nullptr)) {
                const bool nowMoving = (flags & AUHostTransportStateMoving) != 0;
                if (*known && *moving && !nowMoving) { engine->stop(bufferStart); flushStop(); flushSoft(bufferStart); flushTop(bufferStart); }
                *known = true; *moving = nowMoving;engine->observeTransport(nowMoving);
            } else { *known = false;engine->forgetTransport(); }
        }
        engine->beginSourceRetryPass();
        flushSource(bufferStart);
        flushSh01a(bufferStart,static_cast<int64_t>(*elapsedMs));
        if(!transport)engine->forgetTransport();
        // Inspect host transport before retrying an unstarted TOP pulse:
        // a STOP transition must cancel queued ONs, never emit them first.
        flushTop(bufferStart);
        flushMuteLeds(bufferStart,static_cast<int64_t>(*elapsedMs));
        // Host-derived grid supplies authenticated musical edges.
        xone::host::MusicalContext context{};
        context.sampleRate=rate;
        context.moving=(*known && *moving);
        if(context.moving && music) {
            double tempo=0, numerator=0, beat=0, downbeat=0;
            NSInteger denominator=0, nextBeat=0;
            if(music(&tempo,&numerator,&denominator,&beat,&nextBeat,&downbeat)) {
                context.available=true;context.tempo=tempo;context.numerator=numerator;
                context.denominator=static_cast<int>(denominator);
                context.beatPosition=beat;context.measureDownbeat=downbeat;
                context.offsetToNextBeat=static_cast<int64_t>(nextBeat);
            }
        }
        const auto gridObservation=grid->scan(context,frameCount);
        // Dispatch each authenticated BAR in sample order. A BAR at the exact
        // MIDI timestamp wins, so a gesture cannot arm retroactively.
        unsigned gridRead=0;
        const auto deliverGridThrough = [&](AUEventSampleTime sample) {
            while(gridObservation.valid && !gridObservation.overflow &&
                  gridRead<gridObservation.count &&
                  bufferStart+gridObservation.events[gridRead].offset<=sample){
                xone::host::GridBatch one{};
                one.valid=true;one.count=1;one.events[0]=gridObservation.events[gridRead++];
                engine->onMusicalGrid(one);
                flushSoft(bufferStart+one.events[0].offset);
                flushCueLoad(bufferStart+one.events[0].offset);
                flushSource(bufferStart+one.events[0].offset);
                flushSh01a(bufferStart+one.events[0].offset,static_cast<int64_t>(*elapsedMs+1000.0*one.events[0].offset/rate));
                const auto patch=engine->samplePatchOnGrid(one,bufferStart);
                for(unsigned n=0;n<patch.packetCount();++n){
                    const auto packet=patch.packet(n);
                    if(!engine->canSend(packet))continue;
                    if(sendMIDI)engine->sent(packet,sendNominal(packet.sample,packet.api,packet.type,packet.bytes));
                    else engine->noOutput(packet);
                }
                flushMute(bufferStart+one.events[0].offset);
                flushMuteLeds(bufferStart+one.events[0].offset,static_cast<int64_t>(*elapsedMs+1000.0*one.events[0].offset/rate));
            }
        };
        AUEventSampleTime lastEventSample=bufferStart;
        // The host owns a finite, time-ordered list for this render cycle.
        // A fixed count cutoff loses true UP/STOP and cannot be repaired by
        // retaining a host pointer beyond this call. Visit each event once,
        // with no allocation or deferred replay; the trace ring stays bounded.
        for (const AURenderEvent *event = realtimeEventListHead;
             event != nullptr; event = event->head.next) {
            if(event->head.eventType==AURenderEventParameter){
                const auto& parameter=event->parameter;
                const AUEventSampleTime sample=parameter.eventSampleTime==AUEventSampleTimeImmediate?bufferStart:parameter.eventSampleTime;
                if(sample<bufferStart||!frameCount||sample>=bufferStart+frameCount||sample<lastEventSample)continue;
                lastEventSample=sample;deliverGridThrough(sample);
                if(parameter.parameterAddress==0&&std::isfinite(parameter.value)){
                    if(parameter.value>=0.5f)engine->showPresetPage();else engine->leavePresetPage();
                }else if(parameter.parameterAddress==1&&std::isfinite(parameter.value)&&parameter.value>=0.5f){
                    engine->refreshLeds();flushMuteLeds(sample,static_cast<int64_t>(*elapsedMs+1000.0*(sample-bufferStart)/rate));
                }
                continue;
            }
            if (event->head.eventType != AURenderEventMIDI) continue;
            const auto &m = event->MIDI;
            // Immediate is explicitly allowed by AU; other events must belong
            // to this buffer and preserve the host's timestamp ordering.
            const AUEventSampleTime sample = m.eventSampleTime==AUEventSampleTimeImmediate
                ?bufferStart:m.eventSampleTime;
            if(sample<bufferStart || !frameCount || sample>=bufferStart+frameCount || sample<lastEventSample)continue;
            lastEventSample=sample;
            deliverGridThrough(sample);
            if (m.length == 1 && m.data[0] == 0xFC && m.cable == 0) {
                engine->stop(sample); flushStop(); flushSoft(sample); flushTop(sample); flushMuteLeds(sample,static_cast<int64_t>(*elapsedMs+1000.0*(sample-bufferStart)/rate)); continue;
            }
            const auto ms = static_cast<int64_t>(*elapsedMs + 1000.0 * (sample-bufferStart) / rate);
            const auto result = engine->receive(m.cable, m.data, m.length, sample, ms);
            for (unsigned n=0; n<result.packetCount(); ++n) {
                const auto packet = result.packet(n);
                if (!engine->canSend(packet)) continue;
                if (sendMIDI) engine->sent(packet, sendNominal(sample,packet.api,packet.type,packet.bytes));
                else engine->noOutput(packet);
            }
            flushCueLoad(sample);flushSource(sample); // explicit stopped recalls, no synthetic grid
            flushSh01a(sample,ms);
            flushTop(sample); // TOP UP produces pulse at true MIDI offset
            flushMuteLeds(sample,ms);
        }
        if(frameCount)deliverGridThrough(bufferStart+frameCount-1);
        // Rendering progresses while transport is stopped; seeking cannot rewind gestures.
        *elapsedMs += 1000.0 * frameCount / rate;
        engine->advance(static_cast<int64_t>(*elapsedMs), bufferStart+frameCount);
        // SOC-02: the 120 ms SPOT timer may queue the three local FX
        // ON/OFF SourcePackets in advance(), after the MIDI-event flush.
        // Drain them in THIS buffer together with the independently qualified
        // S7 GROUP pulse. Never send at bufferStart+frameCount (out of range).
        if(frameCount){
            flushSource(bufferStart+frameCount-1);
            flushTop(bufferStart+frameCount-1);
        }
        flushMuteLeds(bufferStart+(frameCount?frameCount-1:0),static_cast<int64_t>(*elapsedMs));
        engine->endRenderBuffer();
        // Aucune production audio. Rendre un tampon silencieux si fourni.
        if (outputData) {
            for (UInt32 n = 0; n < outputData->mNumberBuffers; n++) {
                AudioBuffer *b = &outputData->mBuffers[n];
                if (b->mData && b->mDataByteSize) memset(b->mData, 0, b->mDataByteSize);
            }
        }
        return noErr;
    };
}
- (NSString *)drainDiagnosticText {
    // UI main thread only: the sole trace consumer. Formatting stays off render thread.
    static const char *stages[] = {"RX_IN","DECISION","TX_OUT","RX_FEEDBACK","STOP","OVERFLOW","GESTURE"};
    static const char *reasons[] = {"MAPPED","SURFACE_ONLY","REJECTED","UNTRUSTED","SENT","SEND_FAILED","NO_OUTPUT","RECEIVED","INTEGRATED","QUALIFIED","CLOCK_REJECTED","UNKNOWN_LIVE","PICKUP_WAIT","MUSICAL","DJ_RELATIVE","BROWSER_LOAD_INTENT","TOP_RESET_DEFERRED","SH_MIRROR_36","SH_CAPTURE_INCOMPLETE"};
    NSMutableString *text = [NSMutableString stringWithFormat:@"V3 dev | K2/S2/NAV | RX_FEEDBACK=CONFIGURED_PROFILES_ONLY | dropped=%u | overflowBuffers=%u\n", _engine.dropped(), _engine.overflows()];
    xone::host::Trace t{};
    for (unsigned n=0; n<256 && _engine.pop(t); ++n) {
        [text appendFormat:@"%u s=%lld %s %s cable=%u len=%u %02X %02X %02X result=%d",
            t.sequence, (long long)t.sample, stages[(unsigned)t.stage], reasons[(unsigned)t.reason],
            t.cable, t.length, t.bytes[0], t.bytes[1], t.bytes[2], t.result];
        if(t.reason==xone::host::Reason::ShCaptureIncomplete){
            [text appendFormat:@" PC%d %u/36",t.shCapture.program,t.shCapture.count];
            const auto appendMask=[&](NSString *label,const std::array<uint64_t,2>& mask){
                [text appendString:label];
                if(!mask[0]&&!mask[1])[text appendString:@" -"];
                for(unsigned cc=0;cc<128;++cc)
                    if(mask[cc/64]&(uint64_t{1}<<(cc%64)))[text appendFormat:@" %u",cc];
            };
            appendMask(@" missing:",t.shCapture.missing);
            appendMask(@" extra:",t.shCapture.extra);
        }
        [text appendString:@"\n"];
    }
    return text;
}
@end
