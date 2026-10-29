#include "../Shared/XoneHostEngine.hpp"
#include <array>
#include <cassert>
#include <iostream>
using namespace xone::host;
struct Monitor {
 struct Event {int type{},api{},value{};bool success{};};
 std::array<Event,8192> events{};unsigned count{},ledCount{};
 void add(int type,int api,int value,bool success=true){assert(count<events.size());events[count++]={type,api,value,success};}
 bool contains(unsigned from,int type,int api,int value)const {for(unsigned i=from;i<count;++i)if(events[i].success&&events[i].type==type&&events[i].api==api&&events[i].value==value)return true;return false;}
};
struct Rig {
 HostEngine engine;HostGrid clock;Monitor post;int64_t ms{};bool failSource{};
 Rig(){engine.enableNativeMutePage(true);engine.observeTransport(true);MusicalContext c{120,4,0,0,48000,4,0,true,true};assert(clock.scan(c,1).count==0);}
 void begin(){engine.beginRenderBuffer();engine.beginSourceRetryPass();engine.beginCueLoadRetryPass();}
 void flush(){
  for(unsigned n=0;n<32;++n){auto p=engine.nextCueLoadPacket();if(!p.valid)break;assert(p.bytes[0]==0xB6+p.api/128);post.add(17,p.api,p.value);engine.ackCueLoadPacket(p,true,ms*48);}
  for(unsigned n=0;n<2048;++n){auto p=engine.nextSourcePacket();if(!p.valid)break;assert((p.bytes[0]&15)>=6&&(p.bytes[0]&15)<=13);const bool success=!failSource;failSource=false;post.add(p.type,p.api,p.value,success);engine.ackSourcePacket(p,success,ms*48);if(!success)break;}
  engine.prepareMuteLeds(ms);unsigned cursor=0;for(unsigned n=0;n<136;++n){auto p=engine.nextMuteLedPacket(cursor);if(!p.valid)break;assert((p.bytes[0]&15)==14);++post.ledCount;engine.ackMuteLedPacket(p,true);}
 }
 void finish(){flush();engine.endRenderBuffer();}
 void frame(){++ms;begin();engine.advance(ms,ms*48);finish();}
 void midi(uint8_t status,uint8_t number,uint8_t value,int delay=1){ms+=delay;begin();const uint8_t data[]{status,number,value};auto result=engine.receive(0,data,3,ms*48,ms);for(unsigned i=0;i<result.packetCount();++i){auto p=result.packet(i);assert(engine.canSend(p));assert((p.bytes[0]&15)>=6&&(p.bytes[0]&15)<=13);post.add(p.type,p.api,p.value);engine.sent(p,0);}finish();}
 void down(int note){midi(0x9E,static_cast<uint8_t>(note),127);}
 void up(int note,int delay=1){midi(0x8E,static_cast<uint8_t>(note),0,delay);}
 void tap(int note){down(note);up(note);}
 void cc(int number,int value){midi(0xBE,static_cast<uint8_t>(number),static_cast<uint8_t>(value));}
 void grid(double beat){++ms;begin();MusicalContext c{120,4,beat,0,48000,4,0,true,true};const auto batch=clock.scan(c,1);assert(batch.valid&&batch.count==1);engine.onMusicalGrid(batch);finish();}
};
int main(){
 Rig rig;auto& e=rig.engine;rig.frame();assert(rig.post.count==0&&rig.post.ledCount==100);
 assert(!e.desiredLive(320).known&&!e.desiredLive(1328).known&&e.observed()==-2);
 // Verified Golden bytes, no fixture or synthetic NAV: INST1 CUE page, fourth choice BASE.
 rig.tap(24);rig.tap(25);assert(e.sourceRecallStatus(0)==HostEngine::SourceRecallStatus::Pending);
 rig.grid(1);assert(!e.desiredLive(320).known);const unsigned beforeBase=rig.post.count;rig.grid(4);
 assert(rig.post.count==beforeBase+98&&rig.post.contains(beforeBase,17,934,127));
 assert(e.desiredLive(320).value==0&&e.desiredLive(1552).value==0&&e.desiredLive(1768).value==0);
 assert(!e.desiredLive(350).known&&!e.desiredLive(404).known&&!e.desiredLive(1328).known);
 // Known local SOCLE now supports genuine physical SMART and a MEMORY roundtrip.
 rig.tap(36);const unsigned beforeSpot=rig.post.count;rig.down(48);rig.up(48,220);
 assert(rig.post.contains(beforeSpot,11,384,1)&&rig.post.contains(beforeSpot,11,384,0));
 rig.tap(37);assert(e.memoryEngaged(0,0));rig.cc(4,0);rig.cc(4,100);assert(e.desiredLive(1552).value==100);
 rig.tap(37);assert(e.pendingMemorySlot(0)==0&&e.desiredLive(1552).value==100);rig.grid(5);assert(e.desiredLive(1552).value==0);
 // Physical MACHINE mode and common pair SAVE. Modes remain UNKNOWN, outside PATCH60.
 rig.down(13);rig.down(15);rig.up(15);rig.up(13);assert(e.navMode()==7);
 rig.down(14);rig.tap(52);rig.up(14);SourcePatchBank::Patch patch{};assert(e.readSourcePatch(0,0,patch)&&patch.values[0]==0&&patch.values[30]==0);
 rig.cc(4,0);rig.cc(4,73);assert(e.desiredLive(362).value==73);rig.tap(52);assert(e.samplePatchPending());rig.grid(8);
 assert(e.desiredLive(362).value==0&&!e.memoryEngaged(0,0)&&!e.desiredLive(350).known);
 // PRESET uses the actual render-boundary UI intent. RED restore retries nominal MIDI.
 rig.down(13);rig.down(15);rig.up(15);rig.up(13);assert(e.navMode()==0);
 e.requestPresetPage(true);rig.frame();rig.tap(36);assert(e.presetEngaged(0));rig.cc(16,64);rig.cc(16,100);
 rig.tap(12);assert(e.redActive());rig.cc(16,77);rig.failSource=true;rig.tap(12);assert(!e.redActive()&&e.desiredLive(1328).value==100);
 const unsigned beforeRetry=rig.post.count;rig.frame();assert(rig.post.contains(beforeRetry,10,560,100));
 rig.tap(36);const unsigned beforePreset=rig.post.count;rig.grid(12);assert(!e.desiredLive(1328).known&&!rig.post.contains(beforePreset,10,560,0));
 // STOP owns cancellation, preserves banks and ignores a late true UP.
 e.requestPresetPage(false);rig.frame();rig.down(13);rig.down(15);rig.up(15);rig.up(13);assert(e.navMode()==7);rig.tap(52);assert(e.samplePatchPending());rig.down(52);e.stop(rig.ms*48);rig.up(52);assert(!e.samplePatchPending());
 const unsigned stoppedCount=rig.post.count;e.observeTransport(true);rig.grid(16);assert(rig.post.count==stoppedCount);
 auto document=e.documentForHost();DocumentBytes bytes{};assert(encodeDocument(document,bytes));DocumentImage reopened{};assert(decodeDocument(bytes.data(),bytes.size(),reopened));
 HostEngine restored;assert(restored.requestDocumentRestore(reopened));restored.beginRenderBuffer();
 assert(restored.readSourcePatch(0,0,patch)&&patch.values[30]==0&&!restored.desiredLive(1328).known&&!restored.desiredLive(350).known);
 xone::l01::BankS6::MemoryRecord memory{};assert(restored.readMemory(0,0,memory)&&memory.valid&&memory.values[0]==0);
 GlobalSnapshot preset{};assert(restored.readPreset(0,preset)&&preset.valid&&!restored.redActive()&&restored.pendingMemorySlot(0)==-1);
 assert(!restored.nextCueLoadPacket().valid&&!restored.nextTopPulse().valid&&restored.observed()==-2);
 std::cout<<"PASS fresh physical BASE -> SMART/MEMORY -> pair SAVE/RECALL -> PRESET/RED -> STOP -> document reopen, no fixture or external observation\n";
}
