// SOC-02: DRUMS SPOT3 local ON/OFF and Golden GREEN LEDs.
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <array>
#include <iostream>
using namespace xone::host;
static int note(int physical){
 for(int n=40;n<52;++n)if(xone::l01::SurfaceInput::physicalControl(n)==physical)return n;
 return -1;
}
static Result edge(HostEngine& h,int physical,bool down,int ms){
 int n=note(physical);assert(n>=0);
 const uint8_t midi[]{uint8_t(down?0x9e:0x8e),uint8_t(n),uint8_t(down?127:0)};
 return h.receive(0,midi,3,ms,ms);
}
static void flush(HostEngine& h,int target,unsigned expected){
 unsigned count=0;
 for(;;){auto packet=h.nextSourcePacket();if(!packet.valid)break;
  assert(packet.type==11&&packet.value==target);
  h.ackSourcePacket(packet,true,0);++count;
  assert(count<=3);
 }
 assert(count==expected);
}
static void check(HostEngine& h,const std::array<int,3>& cells,
                  const std::array<int,3>& physical,int value){
 const auto image=h.ledImage();
 for(unsigned i=0;i<3;++i){
  const auto live=h.desiredLive(cells[i]);assert(live.known&&live.value==value);
  auto led=image[unsigned(physical[i]-4)];
  assert(led.known&&!led.blinking);
  assert(led.color==(value?MuteLedColor::Green:MuteLedColor::Off));
 }
}
int main(){
 xone::l01::NavS3 nav;assert(nav.initializeDefault());
 auto selected=nav.active();selected.at(297)=1;selected.at(303)=1;
 assert(xone::l01::NavS3::regenerate(selected)&&xone::l01::NavS3::validate(selected));
 std::array<int,3> physical{},cells{},apis{};
 for(int p=20;p<32;++p){
  const auto route=selected.route(p);
  if(route.address>=0&&route.address/1024==3&&(route.address%1024)/32==5&&
     route.position>=0&&route.position<3){
   const unsigned slot=unsigned(route.position);assert(physical[slot]==0);
   physical[slot]=p;
   int cell=-1,preset=-1;
   assert(xone::l01::LiveState::spotAddresses(route.address,route.position,cell,preset));
   cells[slot]=cell;apis[slot]=cell-1384;
  }
 }
 for(int p:physical)assert(p>=20&&p<32);
 SessionImage image{};image.schema=1;image.config=1;image.revision=1;image.desired.fill(-2);
 for(int c:cells)image.desired[unsigned(c)]=0;
 HostEngine h;assert(h.setNavForDiagnostic(selected)&&h.restoreSessionBeforeRendering(image));
 check(h,cells,physical,0);
 // SHORT: only the touched effect; no GROUP pulse in DRUMS.
 auto r=edge(h,physical[0],true,10);assert(r.emit&&r.api==apis[0]);h.sent(r,0);
 r=edge(h,physical[0],false,60);assert(!r.emit);assert(h.desiredLive(cells[0]).value==1);
 assert(!h.nextActionPulse().valid);
 // LONG from ON -> three OFF, physically qualified during hold.
 r=edge(h,physical[0],true,100);assert(!r.emit);
 h.advance(219,219);assert(!h.nextSourcePacket().valid);
 h.advance(220,220);flush(h,0,1);check(h,cells,physical,0);
 assert(!edge(h,physical[0],false,230).emit);
 // LONG from OFF -> three ON. Two peers are emitted after touched DOWN.
 r=edge(h,physical[0],true,300);assert(r.emit);h.sent(r,0);
 h.advance(420,420);flush(h,1,2);check(h,cells,physical,1);
 assert(!edge(h,physical[0],false,450).emit);
 assert(!h.nextActionPulse().valid);
 // Fresh UNKNOWN opening: first physical press still operates the effect.
 HostEngine fresh;assert(fresh.setNavForDiagnostic(selected));
 assert(!fresh.desiredLive(cells[0]).known);
 auto first=edge(fresh,physical[0],true,10);assert(first.emit&&first.api==apis[0]&&first.value==1);
 fresh.sent(first,0);
 assert(fresh.desiredLive(cells[0]).known&&fresh.desiredLive(cells[0]).value==1);
 assert(fresh.ledImage()[unsigned(physical[0]-4)].color==MuteLedColor::Green);
 std::cout<<"PASS SOC-02 DRUMS SPOT3 short/long, 120ms, GREEN LED, UNKNOWN opening, no S7 pulse\n";
}
