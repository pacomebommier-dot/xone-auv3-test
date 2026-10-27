#import "XoneMidiAudioUnit.h"
#import <AVFoundation/AVFoundation.h>
#import <string.h>
#include "../Shared/XoneHostEngine.hpp"
#include "../Shared/XoneHostGrid.hpp"
#include "../Shared/XoneHostScheduler.hpp"

@implementation XoneMidiAudioUnit {
    AUAudioUnitBusArray *_inputBusArray;
    AUAudioUnitBusArray *_outputBusArray;
    // Établi avant de démarrer la boucle de rendu ; ne pas muter en cours de rendu.
    AUMIDIOutputEventBlock _cachedMIDIOutput;
    AUHostTransportStateBlock _cachedTransport;
    AUHostMusicalContextBlock _cachedMusicalContext;
    xone::host::HostGrid _musicalGrid;
    xone::host::HostActionScheduler _musicalScheduler;
    xone::host::HostEngine _engine;
    bool _transportKnown, _moving;
    double _gestureElapsedMs, _gestureSampleRate;
}

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
                                     options:(AudioComponentInstantiationOptions)options
                                       error:(NSError * _Nullable __autoreleasing *)outError {
    self = [super initWithComponentDescription:componentDescription options:options error:outError];
    if (!self) return nil;

    // Busses requis par le contrat AUAudioUnit, même pour un processeur MIDI pur.
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
    return self;
}

- (AUAudioUnitBusArray *)inputBusses { return _inputBusArray; }
- (AUAudioUnitBusArray *)outputBusses { return _outputBusArray; }
- (NSArray<NSString *> *)MIDIOutputNames { return @[@"XONE V0.2 TEST OUT"]; }

- (BOOL)allocateRenderResourcesAndReturnError:(NSError * _Nullable __autoreleasing *)outError {
    if (![super allocateRenderResourcesAndReturnError:outError]) return NO;
    _engine.enableNativeMutePage(true);
    _cachedMIDIOutput = [self.MIDIOutputEventBlock copy];
    _cachedTransport = [self.transportStateBlock copy];
    _cachedMusicalContext = [self.musicalContextBlock copy];
    _musicalGrid.reset();
    _musicalScheduler.stop();
    _transportKnown = false;
    _gestureSampleRate = self.outputBusses[0].format.sampleRate;
    return YES;
}

- (void)deallocateRenderResources {
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
        xone::host::SessionBytes bytes{};
        const auto image = _engine.sessionForHost();
        if (xone::host::encodeSession(image, bytes))
            state[@"XONE_LAST_LIVE_V1"] = [NSData dataWithBytes:bytes.data() length:bytes.size()];
        return state;
    }
}
- (void)setFullState:(NSDictionary<NSString *,id> *)state {
    @synchronized (self) {
        id payload = state[@"XONE_LAST_LIVE_V1"];
        if (payload) {
            if (![payload isKindOfClass:[NSData class]]) return;
            NSData *data = (NSData *)payload;
            xone::host::SessionImage image{};
            if (!xone::host::decodeSession((const uint8_t *)data.bytes, data.length, image)) return;
            if (!_engine.requestSessionRestore(image)) return;
        }
        [super setFullState:state];
    }
}

- (AUInternalRenderBlock)internalRenderBlock {
    // Capturer seulement l'adresse du slot de sortie, pas d'envoi de message à self
    // dans le rendu temps réel. Ne pas allouer ni journaliser dans ce bloc.
    AUMIDIOutputEventBlock __strong *slot = &_cachedMIDIOutput;
    auto *engine = &_engine;
    auto *known = &_transportKnown;
    auto *moving = &_moving;
    auto *elapsedMs = &_gestureElapsedMs;
    const double *sampleRate = &_gestureSampleRate;
    AUHostTransportStateBlock __strong *transportSlot = &_cachedTransport;
    AUHostMusicalContextBlock __strong *musicSlot = &_cachedMusicalContext;
    auto *grid = &_musicalGrid;
    auto *scheduler = &_musicalScheduler;
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
        const auto flushStop = [&]() {
            for (unsigned n=0; n<702; ++n) {
                const auto packet=engine->nextStopPacket();
                if (!packet.emit) break;
                if (!engine->canSend(packet)) continue;
                if (sendMIDI) engine->sent(packet,sendMIDI(packet.sample,0,3,packet.bytes));
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
        const auto flushMuteLeds = [&](AUEventSampleTime sample,int64_t ms) {
            engine->prepareMuteLeds(ms);
            if(!sendMIDI)return;
            unsigned cursor=0;
            for(unsigned n=0;n<64;++n){
                const auto packet=engine->nextMuteLedPacket(cursor);
                if(!packet.valid)break;
                const bool success=sendMIDI(sample,0,3,packet.bytes)==noErr;
                engine->ackMuteLedPacket(packet,success);
                // A failed target retries next render; do not spin or block peers.
                if(!success)cursor=packet.target+1;
            }
        };
        flushMute(bufferStart); // old failed writes, no fabricated BAR
        if (transport) {
            AUHostTransportStateFlags flags = static_cast<AUHostTransportStateFlags>(0);
            if (transport(&flags, nullptr, nullptr, nullptr)) {
                const bool nowMoving = (flags & AUHostTransportStateMoving) != 0;
                if (*known && *moving && !nowMoving) { scheduler->stop(); engine->stop(bufferStart); flushStop(); }
                *known = true; *moving = nowMoving;
            } else { *known = false; }
        }
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
                scheduler->consume(one, [](xone::host::DueAction) noexcept {});
                engine->onMusicalGrid(one);
                flushMute(bufferStart+one.events[0].offset);
                flushMuteLeds(bufferStart+one.events[0].offset,static_cast<int64_t>(*elapsedMs+1000.0*one.events[0].offset/rate));
            }
        };
        AUEventSampleTime lastEventSample=bufferStart;
        unsigned processed = 0;
        const AURenderEvent *event = realtimeEventListHead;
        for (;
             event != nullptr && processed < 256; event = event->head.next, ++processed) {
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
                scheduler->stop(); engine->stop(sample); flushStop(); flushMuteLeds(sample,static_cast<int64_t>(*elapsedMs+1000.0*(sample-bufferStart)/rate)); continue;
            }
            const auto ms = static_cast<int64_t>(*elapsedMs + 1000.0 * (sample-bufferStart) / rate);
            const auto result = engine->receive(m.cable, m.data, m.length, sample, ms);
            for (unsigned n=0; n<result.packetCount(); ++n) {
                const auto packet = result.packet(n);
                if (!engine->canSend(packet)) continue;
                if (sendMIDI) engine->sent(packet, sendMIDI(sample, 0, 3, packet.bytes));
                else engine->noOutput(packet);
            }
            flushMuteLeds(sample,ms);
        }
        if (event != nullptr) engine->renderOverflow(bufferStart);
        else if(frameCount)deliverGridThrough(bufferStart+frameCount-1);
        // Rendering progresses while transport is stopped; seeking cannot rewind gestures.
        *elapsedMs += 1000.0 * frameCount / rate;
        engine->advance(static_cast<int64_t>(*elapsedMs), bufferStart+frameCount);
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
    static const char *reasons[] = {"MAPPED","SURFACE_ONLY","REJECTED","UNTRUSTED","SENT","SEND_FAILED","NO_OUTPUT","RECEIVED","INTEGRATED","QUALIFIED","CLOCK_REJECTED","UNKNOWN_LIVE","PICKUP_WAIT","MUSICAL"};
    NSMutableString *text = [NSMutableString stringWithFormat:@"V3 dev | K2/S2/NAV | RX_FEEDBACK=UNKNOWN | dropped=%u | overflowBuffers=%u\n", _engine.dropped(), _engine.overflows()];
    xone::host::Trace t{};
    for (unsigned n=0; n<256 && _engine.pop(t); ++n) {
        [text appendFormat:@"%u s=%lld %s %s cable=%u len=%u %02X %02X %02X result=%d\n",
            t.sequence, (long long)t.sample, stages[(unsigned)t.stage], reasons[(unsigned)t.reason],
            t.cable, t.length, t.bytes[0], t.bytes[1], t.bytes[2], t.result];
    }
    return text;
}
@end
