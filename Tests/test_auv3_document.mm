#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "../Extension/XoneMidiAudioUnit.h"
#include "../Shared/XoneSession.hpp"
#include <cassert>
#include <array>
#include <iostream>
int main() {
 @autoreleasepool {
    const AudioComponentDescription description={0x61756D69,0x586E3032,0x586E4C62,0,0};
    [AUAudioUnit registerSubclass:[XoneMidiAudioUnit class] asComponentDescription:description name:@"XONE automated host" version:1];
    NSError *error=nil;
    XoneMidiAudioUnit *unit=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(unit && !error);
    // Deliberately fetch the block BEFORE allocation and before configuring 48 kHz.
    auto render=unit.internalRenderBlock;
    AVAudioFormat *format=[[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:48000 channels:2 interleaved:YES];
    assert([unit.inputBusses[0] setFormat:format error:&error]);
    assert([unit.outputBusses[0] setFormat:format error:&error]);
    unit.maximumFramesToRender=8192;
    __block NSUInteger sent=0;
    __block uint8_t lastStatus=0,lastNumber=0,lastValue=0;
    unit.MIDIOutputEventBlock=^OSStatus(AUEventSampleTime time,uint8_t cable,NSInteger length,const uint8_t *data) {
        (void)time;assert(cable==0&&length==3);++sent;lastStatus=data[0];lastNumber=data[1];lastValue=data[2];return noErr;
    };
    xone::host::SessionImage image{};image.schema=1;image.config=1;image.revision=1;image.desired.fill(-2);image.desired[1328]=64;
    xone::host::SessionBytes bytes{};assert(xone::host::encodeSession(image,bytes));
    unit.fullStateForDocument=@{@"XONE_LAST_LIVE_V1":[NSData dataWithBytes:bytes.data() length:bytes.size()]};
    assert(sent==0);
    assert([unit allocateRenderResourcesAndReturnError:&error] && !error);
    assert(sent==0);
    std::array<float,16384> scratch{};
    AudioBufferList output{};output.mNumberBuffers=1;
    output.mBuffers[0].mNumberChannels=2;output.mBuffers[0].mData=scratch.data();
    AUEventSampleTime cursor=0;
    auto pump=[&](AVAudioFrameCount frames,int command=-1,int number=0,int value=0) {
        AudioTimeStamp timestamp{};timestamp.mFlags=kAudioTimeStampSampleTimeValid;timestamp.mSampleTime=static_cast<Float64>(cursor);
        AURenderEvent event{};event.MIDI.eventType=AURenderEventMIDI;event.MIDI.eventSampleTime=cursor;
        event.MIDI.cable=0;event.MIDI.length=static_cast<uint16_t>(command==0xFC?1:3);
        event.MIDI.data[0]=static_cast<uint8_t>(command);event.MIDI.data[1]=static_cast<uint8_t>(number);event.MIDI.data[2]=static_cast<uint8_t>(value);
        AudioUnitRenderActionFlags flags=0;
        scratch.fill(1);output.mBuffers[0].mDataByteSize=frames*2*sizeof(float);
        assert(render(&flags,&timestamp,frames,0,&output,command<0?nullptr:&event,nil)==noErr);
        for(std::size_t i=0;i<2*frames;++i)assert(scratch[i]==0);
        cursor+=frames;
    };
    pump(48,0xBE,16,64);assert(sent==1&&lastStatus==0xBA&&lastNumber==48&&lastValue==64);
    pump(48,0xBE,16,100);assert(sent==2&&lastValue==100);
    NSDictionary *state=unit.fullStateForDocument;
    NSData *saved=state[@"XONE_LAST_LIVE_V1"];assert([saved isKindOfClass:[NSData class]]);
    xone::host::SessionImage decoded{};
    assert(xone::host::decodeSession(static_cast<const uint8_t *>(saved.bytes),saved.length,decoded));
    assert(decoded.desired[1328]==100&&decoded.desired[1329]==-2);
    NSData *generic=unit.fullState[@"XONE_LAST_LIVE_V1"];
    assert([generic isKindOfClass:[NSData class]]);
    assert(xone::host::decodeSession(static_cast<const uint8_t *>(generic.bytes),generic.length,decoded)&&decoded.desired[1328]==100);
    // 48 kHz: age 199 ms must not become LONG; the next millisecond must.
    pump(48,0x9E,13,127);pump(4800);pump(4704);
    NSString *trace=[unit drainDiagnosticText];
    assert([trace containsString:@"01 04 00"] && ![trace containsString:@"01 01 01"]);
    pump(48);trace=[unit drainDiagnosticText];assert([trace containsString:@"01 01 01"]);
    pump(48,0x8E,13,0);trace=[unit drainDiagnosticText];assert([trace containsString:@"01 02 00"]);
    pump(48,0x9E,13,127);(void)[unit drainDiagnosticText];
    pump(48,0xFC);pump(48,0x8E,13,0);trace=[unit drainDiagnosticText];
    assert([trace containsString:@"STOP SURFACE_ONLY"] && ![trace containsString:@"GESTURE QUALIFIED"]);
    assert(sent==2); // no MIDI on document load, gesture NAV or STOP
    NSMutableData *corrupt=[saved mutableCopy];
    static_cast<uint8_t *>(corrupt.mutableBytes)[16+2*1328]^=1; // 100 -> 101, still a legal value
    unit.fullStateForDocument=@{@"XONE_LAST_LIVE_V1":corrupt};
    pump(48);
    NSData *after=unit.fullStateForDocument[@"XONE_LAST_LIVE_V1"];
    assert(xone::host::decodeSession(static_cast<const uint8_t *>(after.bytes),after.length,decoded)&&decoded.desired[1328]==100);
    [unit deallocateRenderResources];
    // A new real AU subclass instance receives the saved document, not the old engine.
    XoneMidiAudioUnit *reopened=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(reopened && !error);reopened.MIDIOutputEventBlock=unit.MIDIOutputEventBlock;
    assert([reopened.inputBusses[0] setFormat:format error:&error]);
    assert([reopened.outputBusses[0] setFormat:format error:&error]);
    reopened.fullState=state; // generic API restore, document API readback
    assert([reopened allocateRenderResourcesAndReturnError:&error]);
    auto reopenedRender=reopened.internalRenderBlock;
    AudioTimeStamp timestamp{};timestamp.mFlags=kAudioTimeStampSampleTimeValid;AudioUnitRenderActionFlags flags=0;
    assert(reopenedRender(&flags,&timestamp,48,0,&output,nullptr,nil)==noErr && sent==2);
    NSData *reloaded=reopened.fullStateForDocument[@"XONE_LAST_LIVE_V1"];
    assert(xone::host::decodeSession(static_cast<const uint8_t *>(reloaded.bytes),reloaded.length,decoded)&&decoded.desired[1328]==100);
    [reopened deallocateRenderResources];
    std::cout<<"PASS real AU wrapper document/recreate round-trip, callback MIDI, 48kHz clock, STOP, corruption, no load TX (macOS harness)\n";
 }
}
