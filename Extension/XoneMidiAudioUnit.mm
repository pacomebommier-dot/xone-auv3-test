#import "XoneMidiAudioUnit.h"
#import <AVFoundation/AVFoundation.h>
#import <string.h>
#include "../Shared/XoneHostEngine.hpp"

@implementation XoneMidiAudioUnit {
    AUAudioUnitBusArray *_inputBusArray;
    AUAudioUnitBusArray *_outputBusArray;
    // Établi avant de démarrer la boucle de rendu ; ne pas muter en cours de rendu.
    AUMIDIOutputEventBlock _cachedMIDIOutput;
    AUHostTransportStateBlock _cachedTransport;
    xone::host::HostEngine _engine;
    bool _transportKnown, _moving;
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
    _cachedMIDIOutput = [self.MIDIOutputEventBlock copy];
    _cachedTransport = [self.transportStateBlock copy];
    _transportKnown = false;
    return YES;
}

- (void)deallocateRenderResources {
    _cachedMIDIOutput = nil;
    _cachedTransport = nil;
    [super deallocateRenderResources];
}

- (AUInternalRenderBlock)internalRenderBlock {
    // Capturer seulement l'adresse du slot de sortie, pas d'envoi de message à self
    // dans le rendu temps réel. Ne pas allouer ni journaliser dans ce bloc.
    AUMIDIOutputEventBlock __strong *slot = &_cachedMIDIOutput;
    auto *engine = &_engine;
    auto *known = &_transportKnown;
    auto *moving = &_moving;
    AUHostTransportStateBlock __strong *transportSlot = &_cachedTransport;
    return ^AUAudioUnitStatus(AudioUnitRenderActionFlags *actionFlags,
                             const AudioTimeStamp *timestamp,
                             AVAudioFrameCount frameCount,
                             NSInteger outputBusNumber,
                             AudioBufferList *outputData,
                             const AURenderEvent *realtimeEventListHead,
                             AURenderPullInputBlock pullInputBlock) {
        (void)actionFlags; (void)outputBusNumber; (void)pullInputBlock;
        AUMIDIOutputEventBlock __unsafe_unretained sendMIDI = *slot;
        AUHostTransportStateBlock __unsafe_unretained transport = *transportSlot;
        const auto bufferStart = static_cast<AUEventSampleTime>(timestamp->mSampleTime);
        if (transport) {
            AUHostTransportStateFlags flags = static_cast<AUHostTransportStateFlags>(0);
            if (transport(&flags, nullptr, nullptr, nullptr)) {
                const bool nowMoving = (flags & AUHostTransportStateMoving) != 0;
                if (*known && *moving && !nowMoving) engine->stop(bufferStart);
                *known = true; *moving = nowMoving;
            } else { *known = false; }
        }
        unsigned processed = 0;
        const AURenderEvent *event = realtimeEventListHead;
        for (;
             event != nullptr && processed < 256; event = event->head.next, ++processed) {
            if (event->head.eventType != AURenderEventMIDI) continue;
            const auto &m = event->MIDI;
            if (m.length == 1 && m.data[0] == 0xFC && m.cable == 0) {
                engine->stop(m.eventSampleTime); continue;
            }
            // Preserve host sample timeline; clamp late/out-of-buffer events.
            AUEventSampleTime sample = m.eventSampleTime;
            if (sample < bufferStart) sample = bufferStart;
            if (frameCount && sample >= bufferStart + frameCount) sample = bufferStart + frameCount - 1;
            const auto result = engine->receive(m.cable, m.data, m.length, sample);
            if (!result.emit) continue;
            if (sendMIDI) engine->sent(result, sendMIDI(sample, 0, 3, result.bytes));
            else engine->noOutput(result);
        }
        if (event != nullptr) engine->renderOverflow(bufferStart);
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
    static const char *stages[] = {"RX_IN","DECISION","TX_OUT","RX_FEEDBACK","STOP","OVERFLOW"};
    static const char *reasons[] = {"MAPPED","SURFACE_ONLY","REJECTED","UNTRUSTED","SENT","SEND_FAILED","NO_OUTPUT"};
    NSMutableString *text = [NSMutableString stringWithFormat:@"V0.2 | RX_FEEDBACK=UNKNOWN | dropped=%u | overflowBuffers=%u\n", _engine.dropped(), _engine.overflows()];
    xone::host::Trace t{};
    for (unsigned n=0; n<256 && _engine.pop(t); ++n) {
        [text appendFormat:@"%u s=%lld %s %s cable=%u len=%u %02X %02X %02X result=%d\n",
            t.sequence, (long long)t.sample, stages[(unsigned)t.stage], reasons[(unsigned)t.reason],
            t.cable, t.length, t.bytes[0], t.bytes[1], t.bytes[2], t.result];
    }
    return text;
}
@end
