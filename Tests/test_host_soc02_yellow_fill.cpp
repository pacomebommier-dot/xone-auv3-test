// SOC-02: four SOCLE source families and the independent S7 GROUP path.
// ROOT1 YELLOW/FILL had previously stolen the physical SPOT DOWN entirely.
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
#include <array>
using namespace xone::host;
static int noteOf(int control){
 for(int n=40;n<52;++n)if(xone::l01::SurfaceInput::physicalControl(n)==control)return n;
 return -1;
}
static Result edge(HostEngine& e,int control,bool down,int ms){
 const int note=noteOf(control);assert(note>=0);
 const uint8_t b[]{uint8_t(down?0x9e:0x8e),uint8_t(note),uint8_t(down?127:0)};
 return e.receive(0,b,3,ms,ms);
}
static xone::l01::NavImage selected(bool fill){
 xone::l01::NavS3 nav;assert(nav.initializeDefault());auto n=nav.active();n.at(291)=1;
 if(fill){n.at(298)=1;n.at(304)=1;}
 else {n.at(295)=1;n.at(301)=1;}
 assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
 return n;
}
static void verify(HostEngine& e,const std::array<int,3>& cells,
                   const std::array<int,3>& controls,int value,MuteLedColor color) {
 const auto view=e.ledImage();
 for(unsigned k=0;k<3;++k){
  auto read=e.desiredLive(cells[k]);assert(read.known&&read.value==value);
  const auto led=view[unsigned(controls[k]-4)];
  assert(led.known&&!led.blinking);
  assert(led.color==(value?color:MuteLedColor::Off));
 }
}
static unsigned flush(HostEngine& e,int value){
 unsigned count=0;
 for(unsigned n=0;n<8;++n){auto p=e.nextSourcePacket();if(!p.valid)break;
  assert(p.type==11&&p.value==value&&p.bytes[2]==(value?127:0));
  e.ackSourcePacket(p,true,0);++count;
 }
 return count;
}
static void pulse(HostEngine& e,int api){
 auto p=e.nextActionPulse();assert(p.valid&&p.api==api&&p.on);
 e.ackTopPulse(p,true,0);p=e.nextActionPulse();
 assert(p.valid&&p.api==api&&!p.on);e.ackTopPulse(p,true,0);
 assert(!e.nextActionPulse().valid);
}
static void leds(HostEngine& e,const std::array<int,3>& controls,bool on,int ms){
 e.beginRenderBuffer();e.prepareMuteLeds(ms);
 unsigned cursor=0,n=0;
 for(unsigned i=0;i<136;++i){
  auto p=e.nextMuteLedPacket(cursor);if(!p.valid)break;
  for(int control:controls)
   if(p.target==unsigned(control-4) && p.color==unsigned(on?1:0) && p.on==on)++n;
  e.ackMuteLedPacket(p,true);
 }
 assert(n==3); // exact physical output for three FX, not only UI snapshot
}
int main(){
 for(bool fill:{false,true}){
  auto n=selected(fill);std::array<int,3> controls{},cells{},apis{};
  const int family=fill?6:4;
  for(int p=20;p<32;++p){
   const auto route=n.route(p);
   if(route.address>=0&&route.address/1024==3&&
      (route.address%1024)/32==family&&route.position>=0&&route.position<3){
    const unsigned k=unsigned(route.position);assert(controls[k]==0);
    controls[k]=p;
    int cell=-1,preset=-1;
    assert(xone::l01::LiveState::spotAddresses(route.address,route.position,cell,preset));
    cells[k]=cell;apis[k]=cell-1384;
   }
  }
  for(int p:controls)assert(p>=20&&p<32);
  SessionImage image{};image.schema=1;image.config=1;image.revision=1;image.desired.fill(-2);
  for(int cell:cells)image.desired[unsigned(cell)]=0;
  HostEngine e;assert(e.setNavForDiagnostic(n)&&e.restoreSessionBeforeRendering(image));
  const auto color=fill?MuteLedColor::Red:MuteLedColor::Yellow;
  verify(e,cells,controls,0,color);
  // A short physical press must reach the individual local FX even though
  // qualified GROUP owns an independent long pulse for YELLOW/FILL.
  auto r=edge(e,controls[0],true,10);assert(r.emit&&r.type==11&&r.api==apis[0]&&r.value==1);e.sent(r,0);
  r=edge(e,controls[0],false,50);assert(!r.emit);
  assert(e.desiredLive(cells[0]).value==1);
  assert(e.desiredLive(cells[1]).value==0&&e.desiredLive(cells[2]).value==0);
  assert(!e.nextActionPulse().valid); // short emits no S7 GROUP pulse
  // Initially ON -> all OFF, at the first timer opportunity after 120ms.
  r=edge(e,controls[0],true,100);assert(!r.emit);
  e.advance(219,219);assert(!e.nextSourcePacket().valid&& !e.nextActionPulse().valid);
  e.advance(220,220);assert(flush(e,0)==1);
  verify(e,cells,controls,0,color);
  pulse(e,fill?632:608);
  r=edge(e,controls[0],false,230);assert(!r.emit);
  // Initially OFF -> all ON, no additional action on UP.
  r=edge(e,controls[0],true,300);assert(r.emit&&r.api==apis[0]);e.sent(r,0);
  e.advance(420,420);assert(flush(e,1)==2);
  verify(e,cells,controls,1,color);
  pulse(e,fill?632:608);
  r=edge(e,controls[0],false,450);assert(!r.emit);
  leds(e,controls,true,450);
  // A mixed group uses the touched SPOT's original state, not majority.
  r=edge(e,controls[1],true,500);assert(!r.emit);
  r=edge(e,controls[1],false,550);assert(r.emit&&r.api==apis[1]&&r.value==0);e.sent(r,0);
  r=edge(e,controls[1],true,600);assert(r.emit&&r.value==1);e.sent(r,0);
  e.advance(720,720);assert(flush(e,1)==0);verify(e,cells,controls,1,color);
  pulse(e,(fill?632:608)+1);
  r=edge(e,controls[1],false,740);assert(!r.emit);
  // Exact UP at 120ms with no advance/timer tick.
  r=edge(e,controls[2],true,800);assert(!r.emit);
  r=edge(e,controls[2],false,920);assert(!r.emit);
  assert(flush(e,0)==3);verify(e,cells,controls,0,color);
  pulse(e,(fill?632:608)+2);
  leds(e,controls,false,920);
 }
 std::cout<<"PASS SOC-02 YELLOW/FILL short vs 120ms long, mixed, S7 pulses and physical LED output\n";
}
