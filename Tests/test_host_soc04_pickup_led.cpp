// SOC-04 — GOLDEN home-made pickup and non-destructive LED overlay.
// A pickup cue must never replace a semantic LED (ON/OFF, blink, RESET).
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace xone::host;
static std::vector<MuteLedOutput::Packet> ledDrain(MuteLedOutput& out){
 std::vector<MuteLedOutput::Packet> packets;unsigned cursor=0;
 for(unsigned n=0;n<136;++n){auto p=out.next(cursor);if(!p.valid)break;
  packets.push_back(p);out.confirm(p,true);}
 return packets;
}
static std::vector<MuteLedOutput::Packet> hostLeds(HostEngine& h,int ms){
 h.prepareMuteLeds(ms);std::vector<MuteLedOutput::Packet> packets;unsigned cursor=0;
 for(unsigned n=0;n<136;++n){auto p=h.nextMuteLedPacket(cursor);if(!p.valid)break;
  packets.push_back(p);h.ackMuteLedPacket(p,true);}
 return packets;
}
static bool has(const std::vector<MuteLedOutput::Packet>& list,unsigned target,unsigned color,bool on){
 for(const auto& p:list)if(p.target==target&&p.color==color&&p.on==on)return true;
 return false;
}
static Result cc(HostEngine& e,int number,int raw,int ms){
 const std::uint8_t b[]{0xBE,static_cast<std::uint8_t>(number),static_cast<std::uint8_t>(raw)};
 return e.receive(0,b,3,48*ms,ms);
}
int main(){
 {
  ContinuousPickup p;bool cue=true;
  assert(p.consider(4,100,64,0,0,&cue)==-1&&!cue);  // far: barraged, no LED
  assert(p.consider(4,100,64,0,10,&cue)==0&&cue);  // same Index + Velocity: GOLDEN bypass
  assert(p.consider(4,100,0,1,20,&cue)==1&&cue);   // latched
  assert(p.consider(4,101,64,1,30,&cue)==-1&&!cue); // new index: re-arm
  assert(p.consider(4,101,64,1,40,&cue)==1&&cue);  // bypass new index only on second packet
  assert(p.consider(4,101,80,2,50,&cue)==-1&&!cue); // NEW value same index: protected rearm
  assert(p.consider(5,200,64,0,0,&cue)==-1&&!cue);
  assert(p.consider(5,200,64,60,1,&cue)==-1&&cue); // first FAST passage: witness, no CC
  assert(p.consider(5,200,64,61,2,&cue)==-1&&!cue); // keep fast direction
  assert(p.consider(5,200,64,60,3,&cue)==60&&cue); // reversal validated
  assert(p.consider(6,300,64,0,0,&cue)==-1&&!cue);
  assert(p.consider(6,300,64,60,200,&cue)==60&&cue); // slow catch: precision 4
  assert(p.consider(7,400,100,0,0,&cue)==-1&&!cue);
  assert(p.consider(7,400,100,20,10,&cue)==-1&&!cue);
  assert(p.consider(7,400,100,10,20,&cue)==-1&&!cue);
  assert(p.consider(7,400,100,20,30,&cue)==-1&&!cue);
  assert(p.consider(7,400,100,10,40,&cue)==91&&cue); // 3 reversals -> glide by ~1/10
 }
 {
  MuteLedOutput out;MuteLedOutput::Image base{};
  base[16]={MuteLedColor::Red,false,true};
  base[17]={MuteLedColor::Green,false,true};
  out.observeFull(base,0);ledDrain(out);
  out.flash(16,MuteLedColor::Red,100);out.observeFull(base,100);
  auto tx=ledDrain(out);assert(tx.size()==1&&has(tx,16,0,false));
  out.flash(17,MuteLedColor::Green,120);
  base[0]={MuteLedColor::Yellow,true,true}; // unrelated context animation
  out.observeFull(base,120);tx=ledDrain(out);
  assert(has(tx,17,2,false)&&!has(tx,16,0,true)); // no global flash cancellation
  out.observeFull(base,149);assert(ledDrain(out).empty());
  out.observeFull(base,150);tx=ledDrain(out);
  assert(has(tx,16,0,true)&&!has(tx,17,2,true));
  out.observeFull(base,170);tx=ledDrain(out);assert(has(tx,17,2,true));
  out.flash(16,MuteLedColor::Red,200);out.observeFull(base,200);
  tx=ledDrain(out);assert(has(tx,16,0,false));
  base[16]={MuteLedColor::Yellow,false,true}; // semantic changes under overlay
  out.observeFull(base,220);tx=ledDrain(out);assert(!has(tx,16,1,true));
  out.observeFull(base,250);tx=ledDrain(out);
  assert(has(tx,16,1,true)&&!has(tx,16,0,true)); // only NEW semantic color restored
  out.flash(18,MuteLedColor::Red,300);out.observeFull(base,300);
  tx=ledDrain(out);assert(has(tx,18,0,true));
  out.observeFull(base,350);tx=ledDrain(out);assert(has(tx,18,0,false));
  out.flash(16,MuteLedColor::Yellow,400);out.cancelFlashes();
  out.observeFull(base,400);tx=ledDrain(out);
  for(const auto& packet:tx)assert(packet.target!=16); // other blinking LEDs remain independent
 }
 {
  HostEngine e;e.beginRenderBuffer();hostLeds(e,0);
  auto r=cc(e,4,0,10);assert(!r.emit);
  assert(hostLeds(e,10).empty()); // far pickup must not flicker
  r=cc(e,4,0,20);assert(r.emit&&r.api==907&&r.value==0);e.sent(r,0);
  auto packets=hostLeds(e,20);
  assert(has(packets,16,0,true)); // default EQ red witness
  r=cc(e,4,5,100);assert(r.emit&&r.api==907&&r.value==5);e.sent(r,0);
  packets=hostLeds(e,100);assert(has(packets,16,0,false));
  packets=hostLeds(e,149);assert(!has(packets,16,0,true));
  packets=hostLeds(e,150);assert(has(packets,16,0,true)); // restore actual EQ LED
 }
 {
  HostEngine borrowed;const std::uint8_t down[]{0x9E,36,127},up[]{0x8E,36,0};
  borrowed.receive(0,down,3,0,0);borrowed.receive(0,up,3,480,10);
  assert(borrowed.navMode()==0);
  assert(borrowed.routeForControl(21).address/1024==1);
  borrowed.beginRenderBuffer();hostLeds(borrowed,0);
  auto r=cc(borrowed,5,0,20);assert(r.emit);borrowed.sent(r,0);
  auto packets=hostLeds(borrowed,20);assert(has(packets,17,0,true)); // borrowed CC visible
  packets=hostLeds(borrowed,70);assert(has(packets,17,0,false)); // no permanent ON
  int note=-1;
  for(int n=40;n<52;++n)if(xone::l01::SurfaceInput::physicalControl(n)==21)note=n;
  assert(note>=0);
  const std::uint8_t press[]{0x9E,static_cast<std::uint8_t>(note),127};
  const std::uint8_t release[]{0x8E,static_cast<std::uint8_t>(note),0};
  borrowed.receive(0,press,3,4800,100);
  packets=hostLeds(borrowed,100);assert(has(packets,17,0,true));
  r=cc(borrowed,5,10,110);if(r.emit)borrowed.sent(r,0);
  assert(hostLeds(borrowed,110).empty()); // RESET LED has priority over CC witness
  borrowed.receive(0,release,3,5760,120);
  packets=hostLeds(borrowed,120);assert(has(packets,17,0,false));
 }
 std::cout<<"PASS SOC-04 GOLDEN pickup duplicate/rapid/slow/glide, LED 50ms layers, borrowed RESET priority\n";
}
