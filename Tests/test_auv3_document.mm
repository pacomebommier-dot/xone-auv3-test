#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "../Extension/XoneMidiAudioUnit.h"
#include "../Shared/XoneSession.hpp"
#include <cassert>
#include <iostream>
int main() {
 @autoreleasepool {
    const AudioComponentDescription description={0x61756D69,0x586E3032,0x586E4C62,0,0};
    [AUAudioUnit registerSubclass:[XoneMidiAudioUnit class] asComponentDescription:description name:@"XONE: Automated Host" version:1];
    NSError *error=nil;
    XoneMidiAudioUnit *unit=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(unit && !error);
    // Deliberately fetch the block BEFORE allocation and before configuring 48 kHz.
    auto render=unit.internalRenderBlock;
    AVAudioFormat *format=[[AVAudioFormat alloc] initStandardFormatWithSampleRate:48000 channels:2];
    assert([unit.inputBusses[0] setFormat:format error:&error]);
    assert([unit.outputBusses[0] setFormat:format error:&error]);
    unit.maximumFramesToRender=8192;
    __block bool running=false;
    unit.transportStateBlock=^BOOL(AUHostTransportStateFlags *flags,double *position,double *cycleStart,double *cycleEnd) {
        (void)position;(void)cycleStart;(void)cycleEnd;
        if (flags) *flags=running?AUHostTransportStateMoving:static_cast<AUHostTransportStateFlags>(0);
        return YES;
    };
    __block NSUInteger sent=0,failAt=0;
    uint8_t bytesSent[64][3]{};auto *recorded=bytesSent;
    __block uint8_t lastStatus=0,lastNumber=0,lastValue=0;
    AUMIDIOutputEventBlock testOutput=^OSStatus(AUEventSampleTime time,uint8_t cable,NSInteger length,const uint8_t *data) {
        (void)time;assert(cable==0&&length==3&&sent<64);
        for(unsigned i=0;i<3;++i)recorded[sent][i]=data[i];
        ++sent;lastStatus=data[0];lastNumber=data[1];lastValue=data[2];return sent==failAt?static_cast<OSStatus>(-1):noErr;
    };
    unit.MIDIOutputEventBlock=testOutput;
    xone::host::SessionImage image{};image.schema=1;image.config=1;image.revision=1;image.desired.fill(-2);image.desired[1328]=64;image.desired[1768]=0;
    image.desired[380]=0;image.desired[392]=3;
    image.desired[381]=0;image.desired[393]=4;
    image.desired[382]=1;image.desired[394]=4;
    xone::host::SessionBytes bytes{};assert(xone::host::encodeSession(image,bytes));
    unit.fullStateForDocument=@{@"XONE_LAST_LIVE_V1":[NSData dataWithBytes:bytes.data() length:bytes.size()]};
    assert(sent==0);
    assert([unit allocateRenderResourcesAndReturnError:&error] && !error);
    assert(sent==0);
    AVAudioPCMBuffer *audio=[[AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:8192];
    assert(audio && audio.floatChannelData);
    AUEventSampleTime cursor=0;
    auto pump=[&](AVAudioFrameCount frames,int command=-1,int number=0,int value=0,AVAudioFrameCount offset=0) {
        AudioTimeStamp timestamp{};timestamp.mFlags=kAudioTimeStampSampleTimeValid;timestamp.mSampleTime=static_cast<Float64>(cursor);
        AURenderEvent event{};event.MIDI.eventType=AURenderEventMIDI;event.MIDI.eventSampleTime=cursor+offset;
        event.MIDI.cable=0;event.MIDI.length=static_cast<uint16_t>(command==0xFC?1:3);
        event.MIDI.data[0]=static_cast<uint8_t>(command);event.MIDI.data[1]=static_cast<uint8_t>(number);event.MIDI.data[2]=static_cast<uint8_t>(value);
        AudioUnitRenderActionFlags flags=0;
        audio.frameLength=frames;
        for(unsigned ch=0;ch<2;++ch)for(unsigned i=0;i<frames;++i)audio.floatChannelData[ch][i]=1;
        assert(render(&flags,&timestamp,frames,0,audio.mutableAudioBufferList,command<0?nullptr:&event,nil)==noErr);
        for(unsigned ch=0;ch<2;++ch)for(unsigned i=0;i<frames;++i)assert(audio.floatChannelData[ch][i]==0);
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
    pump(48);trace=[unit drainDiagnosticText];assert([trace containsString:@"01 01 00"]); // BOT2 long owns MUTE momentary, not legacy PRESET NAV
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
    // Real wrapper: FX selection -> SMART ON -> explicit STOP OFF, true UP stays inert.
    pump(48,0x9E,36,127);pump(48,0x8E,36,0);
    pump(48,0x9E,48,127);assert(sent==3&&lastStatus==0x99&&lastNumber==0&&lastValue==127);
    pump(48,0xFC);assert(sent==4&&lastStatus==0x89&&lastNumber==0&&lastValue==0);
    pump(48,0x8E,48,0);assert(sent==4);
    running=true;pump(48);pump(48,0x9E,48,127);assert(sent==5&&lastStatus==0x99);
    running=false;pump(48);assert(sent==6&&lastStatus==0x89&&lastValue==0);
    pump(48,0x8E,48,0);assert(sent==6);
    // Genuine MACHINE NAV -> TOUCH -> FLASH through the real callback.
    pump(48,0x9E,13,127);pump(48,0x9E,15,127);pump(48,0x8E,15,0);pump(48,0x8E,13,0);
    pump(48,0x9E,48,127);assert(sent==7&&lastStatus==0x96&&lastNumber==108&&lastValue==127);
    pump(48,0x8E,48,0);assert(sent==8&&lastStatus==0x86&&lastNumber==108&&lastValue==0);
    pump(48,0x9E,44,127);assert(sent==11);
    const uint8_t pulseStatus[]{0x86,0x96,0x86};const uint8_t pulseValue[]{0,127,0};
    for(unsigned n=0;n<3;++n)assert(recorded[8+n][0]==pulseStatus[n]&&recorded[8+n][1]==109&&recorded[8+n][2]==pulseValue[n]);
    pump(48,0x8E,44,0);assert(sent==11);
    failAt=12;pump(48,0x9E,40,127);assert(sent==12&&lastStatus==0x86&&lastNumber==110);
    pump(48,0x8E,40,0);assert(sent==12); // failed prerequisite OFF suppresses the pulse
    failAt=0;pump(48,0x9E,40,127);assert(sent==15&&lastStatus==0x86&&lastNumber==110);
    pump(48,0x8E,40,0);assert(sent==15);
    pump(48,0x9E,48,127);assert(sent==16&&lastStatus==0x96&&lastNumber==108);
    failAt=17;pump(48,0x8E,48,0);assert(sent==17&&lastStatus==0x86);
    failAt=0;pump(48,0xFC);assert(sent==18&&lastStatus==0x86&&lastNumber==108); // failed TOUCH OFF is still due
    [unit deallocateRenderResources];
    // A new real AU subclass instance receives the saved document, not the old engine.
    XoneMidiAudioUnit *reopened=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(reopened && !error);reopened.MIDIOutputEventBlock=testOutput;
    assert([reopened.inputBusses[0] setFormat:format error:&error]);
    assert([reopened.outputBusses[0] setFormat:format error:&error]);
    reopened.fullState=state; // generic API restore, document API readback
    assert([reopened allocateRenderResourcesAndReturnError:&error]);
    auto reopenedRender=reopened.internalRenderBlock;
    AudioTimeStamp timestamp{};timestamp.mFlags=kAudioTimeStampSampleTimeValid;AudioUnitRenderActionFlags flags=0;
    assert(reopenedRender(&flags,&timestamp,48,0,audio.mutableAudioBufferList,nullptr,nil)==noErr && sent==18);
    NSData *reloaded=reopened.fullStateForDocument[@"XONE_LAST_LIVE_V1"];
    assert(xone::host::decodeSession(static_cast<const uint8_t *>(reloaded.bytes),reloaded.length,decoded)&&decoded.desired[1328]==100);
    [reopened deallocateRenderResources];
    std::cout<<"AU fixture callback retained="<<(testOutput!=nil)<<" deallocated-unit callback="<<(unit.MIDIOutputEventBlock!=nil)<<"\n";
    // LONG is crossed by an event inside the buffer, before the end-buffer timer.
    XoneMidiAudioUnit *clockUnit=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(clockUnit&&!error);clockUnit.MIDIOutputEventBlock=testOutput;clockUnit.maximumFramesToRender=8192;
    assert([clockUnit.inputBusses[0] setFormat:format error:&error]);
    assert([clockUnit.outputBusses[0] setFormat:format error:&error]);
    clockUnit.fullState=@{@"XONE_LAST_LIVE_V1":[NSData dataWithBytes:bytes.data() length:bytes.size()]};
    assert([clockUnit allocateRenderResourcesAndReturnError:&error]);render=clockUnit.internalRenderBlock;
    pump(48,0x9E,13,127);pump(48,0x9E,15,127);pump(8192);pump(960);
    pump(960,0x9E,48,127,480);
    if(sent!=19||lastStatus!=0x96||lastNumber!=108||lastValue!=127)
        std::cerr<<"in-buffer LONG: sent="<<sent<<" last="<<int(lastStatus)<<","<<int(lastNumber)<<","<<int(lastValue)<<" trace="<<[[clockUnit drainDiagnosticText] UTF8String]<<"\n";
    assert(sent==19&&lastStatus==0x96&&lastNumber==108&&lastValue==127);
    pump(48,0x8E,15,0);pump(48,0x8E,13,0);
    pump(48,0x8E,48,0);assert(sent==20&&lastStatus==0x86&&lastNumber==108&&lastValue==0);
    [clockUnit deallocateRenderResources];

    // Native L06 harness: genuine host 120 BPM / 4:4 BAR, K2 BOT2 SHORT,
    // physical BASE1 SHORT, virtual S7 MUTE Note528 ON exactly at BAR4.
    XoneMidiAudioUnit *muteUnit=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(muteUnit&&!error);muteUnit.MIDIOutputEventBlock=testOutput;
    assert([muteUnit.inputBusses[0] setFormat:format error:&error]);
    assert([muteUnit.outputBusses[0] setFormat:format error:&error]);
    muteUnit.maximumFramesToRender=8192;
    muteUnit.transportStateBlock=^BOOL(AUHostTransportStateFlags *flags,double *position,double *start,double *end){
        (void)position;(void)start;(void)end;
        if(flags)*flags=AUHostTransportStateMoving;
        return YES;
    };
    cursor=0;
    muteUnit.musicalContextBlock=^BOOL(double *tempo,double *numerator,NSInteger *denominator,
                                        double *beatPosition,NSInteger *sampleOffsetToNextBeat,
                                        double *currentMeasureDownbeatPosition){
        if(tempo)*tempo=120;
        if(numerator)*numerator=4;
        if(denominator)*denominator=4;
        if(beatPosition)*beatPosition=static_cast<double>(cursor)/24000.0;
        if(sampleOffsetToNextBeat)*sampleOffsetToNextBeat=static_cast<NSInteger>(
            ((cursor+23999)/24000)*24000-cursor);
        if(currentMeasureDownbeatPosition)*currentMeasureDownbeatPosition=0;
        return YES;
    };
    assert([muteUnit allocateRenderResourcesAndReturnError:&error]);
    render=muteUnit.internalRenderBlock;
    const NSUInteger beforeMute=sent;
    pump(48,0x9E,13,127);
    pump(48,0x8E,13,0);
    pump(48,0x9E,36,127);
    pump(48,0x8E,36,0);
    assert(sent==beforeMute); // no immediate note output on K2 gesture
    for(unsigned n=0;n<12;++n)pump(8192);
    if(sent!=beforeMute+1||lastStatus!=0x9A||lastNumber!=16||lastValue!=127)
        std::cerr<<"MUTE BAR: initial="<<beforeMute<<" sent="<<sent
                 <<" status="<<int(lastStatus)<<" note="<<int(lastNumber)
                 <<" value="<<int(lastValue)
                 <<" trace="<<[[muteUnit drainDiagnosticText] UTF8String]<<"\\n";
    assert(sent==beforeMute+1&&lastStatus==0x9A&&lastNumber==16&&lastValue==127);
    [muteUnit deallocateRenderResources];

    std::cout<<"PASS real AU wrapper document/recreate round-trip, callback MIDI, 48kHz clock, STOP, TOUCH/FLASH callback failures, same-event LONG route, corruption, no load TX (macOS harness)\n";
 }
}
