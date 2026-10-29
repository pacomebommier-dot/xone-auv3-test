#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "../Extension/XoneMidiAudioUnit.h"
#include "../Shared/XoneValidationFixture.hpp"
#include "../Shared/XoneDocument.hpp"
#include "../Shared/XoneCueLoadBar.hpp"
#include "../Core/NativeSlices.hpp"
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
    AUEventSampleTime expectedRange[]{0,8192};auto *expected=expectedRange;
    uint8_t bytesSent[1024][3]{};auto *recorded=bytesSent;
    __block NSUInteger ledSent=0,failLedAt=0;
    uint8_t ledBytes[4096][3]{};auto *ledRecorded=ledBytes;
    __block uint8_t lastStatus=0,lastNumber=0,lastValue=0;
    AUMIDIOutputEventBlock testOutput=^OSStatus(AUEventSampleTime time,uint8_t cable,NSInteger length,const uint8_t *data) {
        assert(time>=expected[0]&&time<expected[1]);assert(cable==0&&length==3);
        if((data[0]&15)==14){
            assert(ledSent<4096&&(data[0]==0x8E||data[0]==0x9E));
            for(unsigned i=0;i<3;++i)ledRecorded[ledSent][i]=data[i];
            ++ledSent;return ledSent==failLedAt?static_cast<OSStatus>(-1):noErr;
        }
        assert(sent<1024);
        for(unsigned i=0;i<3;++i)recorded[sent][i]=data[i];
        ++sent;lastStatus=data[0];lastNumber=data[1];lastValue=data[2];return sent==failAt?static_cast<OSStatus>(-1):noErr;
    };
    unit.MIDIOutputEventBlock=testOutput;
    const auto image=xone::host::aumValidationFixture();
    xone::host::SessionBytes bytes{};assert(xone::host::encodeSession(image,bytes));
    unit.fullStateForDocument=@{@"XONE_LAST_LIVE_V1":[NSData dataWithBytes:bytes.data() length:bytes.size()]};
    assert(sent==0);
    assert([unit allocateRenderResourcesAndReturnError:&error] && !error);
    assert(sent==0);
    AVAudioPCMBuffer *audio=[[AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:8192];
    assert(audio && audio.floatChannelData);
    AUEventSampleTime cursor=0;
    auto pump=[&](AVAudioFrameCount frames,int command=-1,int number=0,int value=0,AUEventSampleTime offset=0) {
        expected[0]=cursor;expected[1]=cursor+frames;
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
    pump(48,0xBE,16,120,-2);assert(sent==2); // late CC cannot be moved to buffer start
    pump(48,0xBE,16,120,48);assert(sent==2); // future CC cannot be moved to buffer end
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
    assert(reopened && !error);
    __block unsigned replayed=0,mixReplayed=0;
    reopened.MIDIOutputEventBlock=^OSStatus(AUEventSampleTime time,uint8_t cable,NSInteger length,const uint8_t *data){
        assert(time>=0&&time<48&&cable==0&&length==3);
        if((data[0]&15)==14)return noErr;
        assert((data[0]&15)>=6&&(data[0]&15)<=13);++replayed;
        if(data[0]==0xBA&&data[1]==48&&data[2]==100)++mixReplayed;
        return noErr;
    };
    assert([reopened.inputBusses[0] setFormat:format error:&error]);
    assert([reopened.outputBusses[0] setFormat:format error:&error]);
    reopened.fullState=state; // generic API restore, document API readback
    assert([reopened allocateRenderResourcesAndReturnError:&error]);
    auto reopenedRender=reopened.internalRenderBlock;
    AudioTimeStamp timestamp{};timestamp.mFlags=kAudioTimeStampSampleTimeValid;AudioUnitRenderActionFlags flags=0;
    assert(reopenedRender(&flags,&timestamp,48,0,audio.mutableAudioBufferList,nullptr,nil)==noErr && sent==18);
    NSData *reloaded=reopened.fullStateForDocument[@"XONE_LAST_LIVE_V1"];
    assert(xone::host::decodeSession(static_cast<const uint8_t *>(reloaded.bytes),reloaded.length,decoded)&&decoded.desired[1328]==100);
    assert(replayed==21&&mixReplayed==1); // Five LIVE values plus 16 applied MUTE states; no historical pulse.
    NSData *documentData=state[@"XONE_DOCUMENT_V2"];assert([documentData isKindOfClass:[NSData class]]);
    xone::host::DocumentImage documentImage{};
    assert(xone::host::decodeDocument((const uint8_t *)documentData.bytes,documentData.length,documentImage));
    assert(documentImage.live.desired[1328]==100);
    NSMutableData *badDocument=[documentData mutableCopy];((uint8_t *)badDocument.mutableBytes)[20]^=1;
    reopened.fullState=@{@"XONE_DOCUMENT_V2":badDocument,@"XONE_LAST_LIVE_V1":saved}; // malformed V2 never falls back to V1
    NSData *stillGood=reopened.fullState[@"XONE_DOCUMENT_V2"];
    assert(xone::host::decodeDocument((const uint8_t *)stillGood.bytes,stillGood.length,documentImage)&&documentImage.live.desired[1328]==100);
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
    // C++ lambda updates cursor; the ObjC block captures a POINTER, not a
    // frozen value. Cursor stays alive until the unit is deallocated.
    const AUEventSampleTime *musicalCursor=&cursor;
    muteUnit.musicalContextBlock=^BOOL(double *tempo,double *numerator,NSInteger *denominator,
                                        double *beatPosition,NSInteger *sampleOffsetToNextBeat,
                                        double *currentMeasureDownbeatPosition){
        if(tempo)*tempo=120;
        if(numerator)*numerator=4;
        if(denominator)*denominator=4;
        if(beatPosition)*beatPosition=static_cast<double>(*musicalCursor)/24000.0;
        if(sampleOffsetToNextBeat)*sampleOffsetToNextBeat=static_cast<NSInteger>(
            ((*musicalCursor+23999)/24000)*24000-*musicalCursor);
        if(currentMeasureDownbeatPosition)*currentMeasureDownbeatPosition=0;
        return YES;
    };
    assert([muteUnit allocateRenderResourcesAndReturnError:&error]);
    render=muteUnit.internalRenderBlock;
    const NSUInteger beforeMute=sent;ledSent=0;
    pump(48,0x9E,37,127);pump(48,0x8E,37,0);assert(ledSent==0); // known underlying red BASE2 (S7 @x4 group3)
    pump(48,0x9E,13,127);
    pump(48,0x8E,13,0);
    assert(ledSent==64&&ledRecorded[3][0]==0x9E&&ledRecorded[3][1]==108);
    failLedAt=66; // fail the first pending red ON after green OFF
    pump(48,0x9E,36,127);
    pump(48,0x8E,36,0);
    assert(ledSent==66&&ledRecorded[64][0]==0x8E&&ledRecorded[64][1]==108&&ledRecorded[65][0]==0x9E&&ledRecorded[65][1]==36);
    failLedAt=0;pump(48);assert(ledSent==70); // three proven OFFs then desired red ON
    assert(ledRecorded[66][0]==0x8E&&ledRecorded[66][1]==36&&ledRecorded[69][0]==0x9E&&ledRecorded[69][1]==36);
    pump(48);assert(ledSent==70); // success clears debt; no duplicate LED ON
    assert(sent==beforeMute); // no immediate note output on K2 gesture
    for(unsigned n=0;n<12;++n)pump(8192);
    if(sent!=beforeMute+1||lastStatus!=0x9A||lastNumber!=16||lastValue!=127)
        std::cerr<<"MUTE BAR: initial="<<beforeMute<<" sent="<<sent
                 <<" status="<<int(lastStatus)<<" note="<<int(lastNumber)
                 <<" value="<<int(lastValue)
                 <<" trace="<<[[muteUnit drainDiagnosticText] UTF8String]<<"\\n";
    assert(sent==beforeMute+1&&lastStatus==0x9A&&lastNumber==16&&lastValue==127);
    // Real AU callback failure at the NEXT host BAR must not acknowledge
    // a global MUTE OFF. Retry exactly once at the next render buffer.
    // This is a MIDI callback acknowledgement, not an AUM effect ACK.
    pump(48,0x9E,36,127);pump(48,0x8E,36,0);
    assert(sent==beforeMute+1); // the new gesture is still queued
    failAt=beforeMute+2;
    for(unsigned n=0;n<12;++n)pump(8192);
    assert(sent==beforeMute+2&&lastStatus==0x8A&&lastNumber==16&&lastValue==0);
    failAt=0;
    pump(48); // pending failed OFF retransmits at render start
    assert(sent==beforeMute+3&&lastStatus==0x8A&&lastNumber==16&&lastValue==0);
    pump(48);assert(sent==beforeMute+3); // successful ACK clears debt
    // A failed ON retries once in the following buffer and stays deduplicated.
    pump(48,0x9E,36,127);pump(48,0x8E,36,0);
    failAt=beforeMute+4;
    for(unsigned n=0;n<12;++n)pump(8192);
    assert(sent==beforeMute+4&&lastStatus==0x9A&&lastNumber==16);
    failAt=0;pump(48);assert(sent==beforeMute+5&&lastStatus==0x9A);
    pump(48);assert(sent==beforeMute+5);
    // BAR wins an exact timestamp tie: an UP at BAR arms the following BAR.
    const auto until=[&](AUEventSampleTime target){
        while(cursor<target){const auto left=target-cursor;pump(static_cast<AVAudioFrameCount>(left>8192?8192:left));}
    };
    auto nextBar=((cursor/96000)+1)*96000;
    until(nextBar-96);pump(48,0x9E,37,127);
    const NSUInteger beforeTie=sent;
    pump(96,0x8E,37,0,48);assert(sent==beforeTie);
    // STOP one sample before the next BAR cancels this pending BASE2.
    nextBar+=96000;until(nextBar-48);
    pump(96,0xFC,0,0,47);assert(sent==beforeTie);
    // A pending BASE3 at a BAR/STOP tie applies BAR first, then STOP.
    pump(48,0x9E,38,127);pump(48,0x8E,38,0);
    nextBar+=96000;until(nextBar-48);
    pump(96,0xFC,0,0,48);
    assert(sent==beforeTie+1&&lastStatus==0x9A&&lastNumber==18);
    pump(48);assert(sent==beforeTie+1);
    const NSUInteger beforeRestore=ledSent;
    pump(48,0x9E,13,127);pump(48,0x8E,13,0);
    bool restoredBase2=false;
    for(NSUInteger i=beforeRestore;i<ledSent;++i)
        if(ledRecorded[i][0]==0x9E){assert(ledRecorded[i][1]==37&&ledRecorded[i][2]==127);restoredBase2=true;}
    assert(restoredBase2&&sent==beforeTie+1); // known context restored, music untouched
    const NSUInteger afterRestore=ledSent;pump(48);assert(ledSent==afterRestore);
    [muteUnit deallocateRenderResources];

    // Fresh AU starts UNKNOWN. Loading an explicit simulated recipe uses
    // the same session mailbox as document restore, and never transmits on load.
    XoneMidiAudioUnit *lifecycle=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(lifecycle&&!error);lifecycle.MIDIOutputEventBlock=testOutput;lifecycle.maximumFramesToRender=8192;
    assert([lifecycle.inputBusses[0] setFormat:format error:&error]);
    assert([lifecycle.outputBusses[0] setFormat:format error:&error]);
    assert(lifecycle.fullState[@"XONE_LAST_LIVE_V1"]==nil); // no valid configured session yet
    const NSUInteger beforeFixture=sent;
    assert([lifecycle allocateRenderResourcesAndReturnError:&error]);render=lifecycle.internalRenderBlock;
    // Real AU, no saved fixture and no synth: physical CC is first DESIRED.
    pump(48,0xBE,16,64);
    assert(sent==beforeFixture+1&&lastStatus==0xBA&&lastNumber==48&&lastValue==64);
    assert([lifecycle loadAumValidationFixture]);assert(sent==beforeFixture+1);
    pump(48,0xBE,16,0);assert(sent==beforeFixture+1); // restored 64 requires pickup
    pump(4800);pump(4800);
    pump(48,0xBE,16,64);assert(sent==beforeFixture+2&&lastStatus==0xBA&&lastNumber==48&&lastValue==64);
    pump(48,0x9E,36,127);pump(48,0x8E,36,0);
    pump(48,0x9E,48,127);assert(sent==beforeFixture+3&&lastStatus==0x99);
    [lifecycle deallocateRenderResources];
    assert(sent==beforeFixture+3); // no callback outside the host render cycle
    assert([lifecycle allocateRenderResourcesAndReturnError:&error]);
    pump(48);assert(sent==beforeFixture+3); // superclass cleared output; debt stays due
    [lifecycle deallocateRenderResources];
    lifecycle.MIDIOutputEventBlock=testOutput; // host reconnects the output before allocation
    assert([lifecycle allocateRenderResourcesAndReturnError:&error]);
    pump(48);assert(sent==beforeFixture+4&&lastStatus==0x89&&lastNumber==0&&lastValue==0);
    pump(48,0x8E,48,0);assert(sent==beforeFixture+4); // real UP after shutdown stays inert
    [lifecycle deallocateRenderResources];
    lifecycle.MIDIOutputEventBlock=testOutput;
    assert([lifecycle allocateRenderResourcesAndReturnError:&error]);
    pump(48);assert(sent==beforeFixture+4); // confirmed OFF is never duplicated
    [lifecycle deallocateRenderResources];

    // Real AU without fixture or plugin: K2 TOP1 physical UP produces
    // Note680 ON+OFF at the same sample offset in MIDI12. Callback failure
    // on ON still incurs an OFF; a failed OFF must retry next render.
    XoneMidiAudioUnit *topUnit=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(topUnit&&!error);topUnit.MIDIOutputEventBlock=testOutput;
    assert([topUnit.inputBusses[0] setFormat:format error:&error]);
    assert([topUnit.outputBusses[0] setFormat:format error:&error]);
    topUnit.maximumFramesToRender=8192;
    assert([topUnit allocateRenderResourcesAndReturnError:&error]);render=topUnit.internalRenderBlock;
    const NSUInteger beforeTop=sent;
    pump(48,0x9E,52,127);assert(sent==beforeTop);
    failAt=beforeTop+1;
    pump(48,0x8E,52,0,24); // failed ON cannot erase the required OFF
    assert(sent==beforeTop+2 && recorded[beforeTop][0]==0x9B &&
           recorded[beforeTop][1]==40 && recorded[beforeTop][2]==127 &&
           recorded[beforeTop+1][0]==0x8B && recorded[beforeTop+1][1]==40 &&
           recorded[beforeTop+1][2]==0);
    failAt=0;
    pump(48);assert(sent==beforeTop+2); // no duplicate after successful OFF
    pump(48,0x9E,53,127);
    failAt=beforeTop+4;
    pump(48,0x8E,53,0,12);
    assert(sent==beforeTop+4&&recorded[beforeTop+2][0]==0x9B&&
           recorded[beforeTop+2][1]==41&&recorded[beforeTop+3][0]==0x8B);
    failAt=0;
    pump(48);assert(sent==beforeTop+5&&lastStatus==0x8B&&lastNumber==41&&lastValue==0);
    pump(48);assert(sent==beforeTop+5);
    [topUnit deallocateRenderResources];

    // Real AU: no fixture, physical CUE4 BASE -> local owner0 at host BAR.
    // One rejected callback retries only in the following render buffer.
    XoneMidiAudioUnit *sourceUnit=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(sourceUnit&&!error);sourceUnit.MIDIOutputEventBlock=testOutput;
    assert([sourceUnit.inputBusses[0] setFormat:format error:&error]);
    assert([sourceUnit.outputBusses[0] setFormat:format error:&error]);
    sourceUnit.maximumFramesToRender=8192;cursor=0;
    sourceUnit.transportStateBlock=^BOOL(AUHostTransportStateFlags *flags,double *position,double *start,double *end){
        (void)position;(void)start;(void)end;if(flags)*flags=AUHostTransportStateMoving;return YES;
    };
    sourceUnit.musicalContextBlock=^BOOL(double *tempo,double *numerator,NSInteger *denominator,double *beatPosition,NSInteger *offset,double *downbeat){
        if(tempo)*tempo=120;
        if(numerator)*numerator=4;
        if(denominator)*denominator=4;
        if(beatPosition)*beatPosition=static_cast<double>(*musicalCursor)/24000.0;
        if(offset)*offset=static_cast<NSInteger>(((*musicalCursor+23999)/24000)*24000-*musicalCursor);
        if(downbeat)*downbeat=0;
        return YES;
    };
    assert([sourceUnit allocateRenderResourcesAndReturnError:&error]);render=sourceUnit.internalRenderBlock;
    const auto physicalNote=[](int physical){
        for(int n=24;n<40;++n)if(xone::l01::SurfaceInput::physicalControl(n)==physical)return n;
        assert(false);return -1;
    };
    xone::l01::NavS3 sourceNav;assert(sourceNav.initializeDefault());int selectCue=-1;
    for(int p=4;p<20;++p)if(sourceNav.active().route(p).address==2*1024+3*32+4)selectCue=p;
    assert(selectCue>=0);pump(48,0x9E,physicalNote(selectCue),127);pump(48,0x8E,physicalNote(selectCue),0);
    auto cueNav=sourceNav.active();cueNav.at(299)=1;cueNav.at(293)=4;
    assert(xone::l01::NavS3::regenerate(cueNav)&&sourceNav.stage(cueNav)&&sourceNav.commit());int baseButton=-1;
    for(int p=4;p<20;++p){unsigned group=0,index=0;
        if(xone::host::CueLoadBar::derive(sourceNav,static_cast<unsigned>(p),group,index)&&group==0&&index==3)baseButton=p;
    }
    assert(baseButton>=0);const NSUInteger beforeSource=sent;
    pump(48,0x9E,physicalNote(baseButton),127);pump(48,0x8E,physicalNote(baseButton),0);assert(sent==beforeSource);
    failAt=beforeSource+2;
    for(unsigned n=0;n<12;++n)pump(8192);
    assert(sent==beforeSource+2); // CUE CC then failed first nominal CC; no same-buffer retry
    assert(recorded[beforeSource][0]==0xBD&&recorded[beforeSource][1]==38&&recorded[beforeSource][2]==127);
    assert(recorded[beforeSource+1][0]==0xB7&&recorded[beforeSource+1][1]==0&&recorded[beforeSource+1][2]==0);
    failAt=0;pump(48);assert(sent==beforeSource+99);pump(48);assert(sent==beforeSource+99);
    NSData *sourceState=sourceUnit.fullStateForDocument[@"XONE_LAST_LIVE_V1"];
    assert(xone::host::decodeSession(static_cast<const uint8_t *>(sourceState.bytes),sourceState.length,decoded));
    assert(decoded.desired[320]==0&&decoded.desired[1552]==0&&decoded.desired[1768]==0);
    assert(decoded.desired[350]==-2&&decoded.desired[1328]==-2&&decoded.desired[404]==-2);
    [sourceUnit deallocateRenderResources];

    // Physical RED: capture on first short press, immediate restore on second.
    // Native global recall failures use the same next-render retry boundary.
    XoneMidiAudioUnit *redUnit=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(redUnit&&!error);redUnit.MIDIOutputEventBlock=testOutput;
    assert([redUnit.inputBusses[0] setFormat:format error:&error]);
    assert([redUnit.outputBusses[0] setFormat:format error:&error]);redUnit.maximumFramesToRender=8192;
    auto redSession=image;redSession.desired.fill(-2);redSession.desired[1328]=64;
    assert(xone::host::encodeSession(redSession,bytes));
    redUnit.fullState=@{@"XONE_LAST_LIVE_V1":[NSData dataWithBytes:bytes.data() length:bytes.size()]};
    assert([redUnit allocateRenderResourcesAndReturnError:&error]);render=redUnit.internalRenderBlock;
    const NSUInteger beforeRed=sent;cursor=0;
    pump(48,0x9E,12,127);pump(48,0x8E,12,0);assert(sent==beforeRed);
    pump(48,0xBE,16,64);pump(48,0xBE,16,100);assert(sent==beforeRed+2);
    pump(48,0x9E,12,127);failAt=beforeRed+3;pump(48,0x8E,12,0);
    assert(sent==beforeRed+3&&lastStatus==0xBA&&lastNumber==48&&lastValue==64);
    failAt=0;pump(48);assert(sent==beforeRed+4);pump(48);assert(sent==beforeRed+4);
    NSData *redState=redUnit.fullStateForDocument[@"XONE_LAST_LIVE_V1"];
    assert(xone::host::decodeSession(static_cast<const uint8_t *>(redState.bytes),redState.length,decoded));
    assert(decoded.desired[1328]==64&&decoded.desired[0]==-2);
    [redUnit deallocateRenderResources];

    // Generic AU parameter opens the PRESET face; BOT2 keeps its MUTE role.
    XoneMidiAudioUnit *presetUnit=[[XoneMidiAudioUnit alloc] initWithComponentDescription:description options:0 error:&error];
    assert(presetUnit&&!error);presetUnit.MIDIOutputEventBlock=testOutput;
    assert([presetUnit.inputBusses[0] setFormat:format error:&error]);
    assert([presetUnit.outputBusses[0] setFormat:format error:&error]);presetUnit.maximumFramesToRender=8192;
    assert(xone::host::encodeSession(redSession,bytes));presetUnit.fullState=@{@"XONE_LAST_LIVE_V1":[NSData dataWithBytes:bytes.data() length:bytes.size()]};
    presetUnit.transportStateBlock=^BOOL(AUHostTransportStateFlags *flags,double *position,double *start,double *end){
        (void)position;(void)start;(void)end;if(flags)*flags=AUHostTransportStateMoving;return YES;
    };
    presetUnit.musicalContextBlock=^BOOL(double *tempo,double *numerator,NSInteger *denominator,double *beatPosition,NSInteger *offset,double *downbeat){
        if(tempo)*tempo=120;
        if(numerator)*numerator=4;
        if(denominator)*denominator=4;
        if(beatPosition)*beatPosition=static_cast<double>(*musicalCursor)/24000.0;
        if(offset)*offset=static_cast<NSInteger>(((*musicalCursor+23999)/24000)*24000-*musicalCursor);
        if(downbeat)*downbeat=0;
        return YES;
    };
    AUParameter *face=[presetUnit.parameterTree parameterWithAddress:0];assert(face&&presetUnit.parameterTree.allParameters.count==1);face.value=1;
    assert([presetUnit allocateRenderResourcesAndReturnError:&error]);render=presetUnit.internalRenderBlock;cursor=0;
    const NSUInteger beforePreset=sent;pump(48);assert(sent==beforePreset);
    auto faceValue=presetUnit.parameterTree.implementorValueProvider;assert(faceValue(face)==1);
    pump(48,0x9E,36,127);pump(48,0x8E,36,0);assert(sent==beforePreset);
    pump(48,0xBE,16,64);pump(48,0xBE,16,100);assert(sent==beforePreset+2);
    pump(48,0x9E,36,127);pump(48,0x8E,36,0);assert(sent==beforePreset+2);
    for(unsigned i=0;i<12;++i)pump(8192);
    assert(sent==beforePreset+3&&lastStatus==0xBA&&lastNumber==48&&lastValue==64);
    NSData *presetState=presetUnit.fullStateForDocument[@"XONE_LAST_LIVE_V1"];
    assert(xone::host::decodeSession(static_cast<const uint8_t *>(presetState.bytes),presetState.length,decoded)&&decoded.desired[1328]==64);
    face.value=0;pump(48);assert(faceValue(face)==0);
    // Host automation reaches S3 through the realtime parameter-event path too.
    expected[0]=cursor;expected[1]=cursor+48;
    AudioTimeStamp parameterTime{};parameterTime.mFlags=kAudioTimeStampSampleTimeValid;parameterTime.mSampleTime=static_cast<Float64>(cursor);
    AURenderEvent parameterEvent{};parameterEvent.parameter.eventType=AURenderEventParameter;parameterEvent.parameter.eventSampleTime=cursor+12;
    parameterEvent.parameter.parameterAddress=0;parameterEvent.parameter.value=1;
    AudioUnitRenderActionFlags parameterFlags=0;audio.frameLength=48;
    assert(render(&parameterFlags,&parameterTime,48,0,audio.mutableAudioBufferList,&parameterEvent,nil)==noErr);cursor+=48;
    assert(faceValue(face)==1&&sent==beforePreset+3);
    [presetUnit deallocateRenderResources];

    std::cout<<"PASS real AU wrapper document/recreate round-trip, callback MIDI, 48kHz clock, STOP, TOUCH/FLASH callback failures, same-event LONG route, corruption, no load TX (macOS harness)\n";
 }
}
