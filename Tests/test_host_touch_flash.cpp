#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace xone::host;
struct Bytes {int status,number,value;};
static SessionImage image(int mode,int state=0){SessionImage i{};i.schema=1;i.config=1;i.revision=1;i.desired.fill(-2);i.desired[380]=state;i.desired[392]=mode;return i;}
static Result edge(HostEngine& e,int note,bool down,int ms){const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};return e.receive(0,b,3,ms,ms);}
static void machine(HostEngine& e){edge(e,13,true,0);edge(e,15,true,10);edge(e,15,false,20);edge(e,13,false,30);assert(e.navMode()==7);}
// Uses the same callback protocol as the real AU adapter, including every packet gate.
static std::vector<Bytes> deliver(HostEngine& e,const Result& r,int failAt=-1,bool absent=false){
 std::vector<Bytes> attempted;
 for(unsigned n=0;n<r.packetCount();++n){auto p=r.packet(n);if(!e.canSend(p))continue;
  if(absent){e.noOutput(p);continue;}
  attempted.push_back({p.bytes[0],p.bytes[1],p.bytes[2]});e.sent(p,static_cast<int>(n)==failAt?-1:0);
 }
 return attempted;
}
static void offOnly(const std::vector<Bytes>& bytes){assert(bytes.size()==1&&bytes[0].status==0x86&&bytes[0].number==108&&bytes[0].value==0);}
int main(){
 for(int initial:{0,1}){
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(3,initial)));machine(e);
  auto on=edge(e,48,true,40);auto bytes=deliver(e,on);assert(bytes.size()==1&&bytes[0].status==0x96&&bytes[0].number==108&&bytes[0].value==127);
  assert(e.desiredLive(380).value==1);assert(!edge(e,48,true,41).emit);
  // Physical selection cannot change the captured release address.
  edge(e,36,true,42);edge(e,36,false,43);
  offOnly(deliver(e,edge(e,48,false,44)));assert(e.desiredLive(380).value==0);
  e.stop(45);assert(!e.nextStopPacket().emit);assert(!edge(e,48,false,46).emit);
 }
 for(int initial:{0,1}){
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(4,initial)));machine(e);
  auto flash=edge(e,48,true,40);assert(flash.packetCount()==3);
  auto bytes=deliver(e,flash);assert(bytes.size()==3);
  const int statuses[]{0x86,0x96,0x86};const int values[]{0,127,0};
  for(unsigned n=0;n<3;++n)assert(bytes[n].status==statuses[n]&&bytes[n].number==108&&bytes[n].value==values[n]);
  assert(e.desiredLive(380).value==0);assert(!edge(e,48,true,41).emit);assert(!edge(e,48,false,42).emit);
  bytes=deliver(e,edge(e,48,true,43));assert(bytes.size()==2&&bytes[0].status==0x96&&bytes[1].status==0x86); // a new pulse is never deduplicated
  assert(!edge(e,48,false,44).emit);e.stop(45);assert(!e.nextStopPacket().emit);
 }
 // S2 qualifies LONG before this same SPOT input; capture the newly qualified NAV.
 {
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(3)));
  edge(e,13,true,0);edge(e,15,true,10);
  auto on=edge(e,48,true,220);assert(e.navMode()==7&&e.longNavPending());
  auto bytes=deliver(e,on);assert(bytes.size()==1&&bytes[0].status==0x96&&bytes[0].number==108);
  edge(e,15,false,230);edge(e,13,false,240);assert(e.navMode()==0&&!e.longNavPending());
  offOnly(deliver(e,edge(e,48,false,250)));assert(e.desiredLive(380).value==0);
 }
 // No FLASH ON after failed nominal OFF. No later replay, even after unrelated success.
 {
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(4,1)));machine(e);
  auto flash=edge(e,48,true,40);offOnly(deliver(e,flash,0));assert(!e.canSend(flash.packet(1)));
  e.stop(41);assert(!e.nextStopPacket().emit);edge(e,48,false,42);
  auto next=edge(e,48,true,43);assert(deliver(e,next).size()==3);assert(!e.canSend(flash.packet(1)));
 }
 // A failing FLASH ON may have escaped: its OFF must still run.
 {
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(4)));machine(e);
  assert(deliver(e,edge(e,48,true,40),1).size()==3);e.stop(41);assert(!e.nextStopPacket().emit);
 }
 // STOP between preparation and delivery cancels unsent ON, never an owed OFF.
 {
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(4)));machine(e);
  auto pending=edge(e,48,true,40);e.sent(pending.packet(0),0);assert(e.canSend(pending.packet(1)));
  e.stop(41);assert(!e.canSend(pending.packet(1)));assert(!e.nextStopPacket().emit);
 }
 // Failed pulse OFF retains its physical owner and debt, until a confirmed STOP OFF.
 {
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(4)));machine(e);
  assert(deliver(e,edge(e,48,true,40),2).size()==3);assert(!edge(e,48,false,41).emit);
  e.stop(42);offOnly(deliver(e,e.nextStopPacket(),0));e.stop(43);offOnly(deliver(e,e.nextStopPacket()));assert(!e.nextStopPacket().emit);
 }
 // Failed TOUCH ON and OFF, STOP while held, and true UP without a ghost ON.
 {
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(3)));machine(e);
  assert(deliver(e,edge(e,48,true,40),0).size()==1);e.stop(41);assert(e.surfaceHeld(20)&&e.desiredLive(380).value==0);
  offOnly(deliver(e,e.nextStopPacket(),0));assert(!edge(e,48,false,42).emit);
  e.stop(43);offOnly(deliver(e,e.nextStopPacket()));assert(!e.nextStopPacket().emit);
  auto on=edge(e,48,true,44);deliver(e,on);offOnly(deliver(e,edge(e,48,false,45),0));
  // No new TOUCH may steal a previously failed release.
  assert(!edge(e,48,true,46).emit);e.stop(47);offOnly(deliver(e,e.nextStopPacket()));assert(!edge(e,48,false,48).emit);
 }
 // Missing callback creates no fictional sent pulse or owned note.
 for(int mode:{3,4}){
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(mode)));machine(e);
  assert(deliver(e,edge(e,48,true,40),-1,true).empty());deliver(e,edge(e,48,false,41),-1,true);
  e.stop(42);assert(!e.nextStopPacket().emit);
 }
 // Session reload waits for failed OFF even after the physical UP has arrived.
 {
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(3)));machine(e);deliver(e,edge(e,48,true,40));
  offOnly(deliver(e,edge(e,48,false,41),0));auto restored=image(4,1);restored.revision=7;
  assert(e.requestSessionRestore(restored));e.beginRenderBuffer();assert(e.desiredLive(380).value==0);
  e.stop(42);offOnly(deliver(e,e.nextStopPacket()));e.beginRenderBuffer();assert(e.desiredLive(380).value==1);
 }
 // At the LIVE revision bound, an owned TOUCH OFF remains owed and must not restore ON.
 {
  HostEngine e;auto i=image(3);i.revision=xone::l01::LiveState::MaxRevision-1;
  assert(e.restoreSessionBeforeRendering(i));machine(e);deliver(e,edge(e,48,true,40));
  offOnly(deliver(e,edge(e,48,false,41)));assert(e.desiredLive(380).value==1);
  e.stop(42);assert(!e.nextStopPacket().emit);
 }
 for(int mode:{3,4}){
  HostEngine e;assert(e.restoreSessionBeforeRendering(image(mode,-2)));machine(e);
  assert(!edge(e,48,true,40).emit&&!edge(e,48,false,41).emit);assert(!e.desiredLive(380).known);
 }
 std::cout<<"PASS TOUCH/FLASH source sequences, ownership, conditional pulse, failed ON/OFF, STOP, revision bound, deferred restore, UNKNOWN\n";
}
