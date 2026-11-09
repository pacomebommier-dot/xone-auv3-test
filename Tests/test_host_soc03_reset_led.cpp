// SOC-03: borrowed RESET SPOT, 120ms 3-CC group and momentary K2 LED.
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <array>
#include <iostream>
using namespace xone::host;
static int note(int physical){
 for(int n=40;n<52;++n)if(xone::l01::SurfaceInput::physicalControl(n)==physical)return n;
 return -1;
}
static void edge(HostEngine& h,int physical,bool down,int ms){
 const int n=note(physical);assert(n>=0);
 const uint8_t midi[]{uint8_t(down?0x9E:0x8E),uint8_t(n),uint8_t(down?127:0)};
 const auto r=h.receive(0,midi,3,ms*48,ms);
 assert(!r.emit);
}
static xone::l01::NavImage selected(int family){
 xone::l01::NavS3 nav;assert(nav.initializeDefault());auto n=nav.active();
 const int root=(family==4||family==6)?1:0;n.at(291)=root;
 if(family==3||family==4){n.at(293+2*root)=1;n.at(299+2*root)=1;}
 else if(family==5||family==6){n.at(297+root)=1;n.at(303+root)=1;}
 assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
 return n;
}
static void flush(HostEngine& e,const std::array<int,3>& apis,int expected,unsigned count){
 unsigned found=0;
 for(unsigned n=0;n<10;++n){auto p=e.nextSourcePacket();if(!p.valid)break;
  assert(p.type==10&&p.value==expected&&p.bytes[2]==expected);
  bool match=false;for(int api:apis)if(p.api==api)match=true;
  assert(match);e.ackSourcePacket(p,true,0);++found;
 }
 assert(found==count);
}
static void inspect(HostEngine& e,const std::array<int,3>& physical,
                    MuteLedColor color,int ms,bool expectedOn){
 const auto view=e.ledImage(ms);
 for(int p:physical){
  const auto led=view[unsigned(p-4)];
  assert(!led.blinking);
  if(expectedOn)assert(led.known&&led.color==color);
  else assert(!led.known||led.color==MuteLedColor::Off);
 }
}
int main(){
 for(int family=3;family<=8;++family){
  // INST/YELLOW borrowed 3..6, DRUMS/FILL borrowed 5/6;
  // also EQ without active page on each ROOT (7/8).
  const bool eq=family>=7;
  xone::l01::NavImage nav;
  if(eq){xone::l01::NavS3 base;assert(base.initializeDefault());nav=base.active();
   nav.at(291)=family-7;
   assert(xone::l01::NavS3::regenerate(nav)&&xone::l01::NavS3::validate(nav));
  }else nav=selected(family);
  std::array<int,3> physical{},cells{},apis{};
  const int column=eq?0:((family==3||family==4)?1:3);
  for(int row=0;row<3;++row){
   int p=20+column+4*row;
   const auto r=nav.route(p);
   assert(r.address>=0&&r.address/1024==(eq?9:1));
   const auto target=continuousTarget({r.address-(r.address/1024)*1024,r.position});
   assert(target.valid());physical[unsigned(row)]=p;cells[unsigned(row)]=target.cell;apis[unsigned(row)]=target.api;
  }
  SessionImage image{};image.schema=1;image.config=1;image.revision=1;image.desired.fill(-2);
  for(int cell:cells)image.desired[unsigned(cell)]=64;
  HostEngine h;assert(h.setNavForDiagnostic(nav)&&h.restoreSessionBeforeRendering(image));
  const int value=eq?63:(family<=4?0:0);
  const auto color=(family==4||family==8)?MuteLedColor::Yellow:
                   family==5?MuteLedColor::Green:MuteLedColor::Red;
  // Short one: immediate CC and temporary LED only while held.
  edge(h,physical[0],true,10);flush(h,apis,value,1);
  assert(h.desiredLive(cells[0]).value==value);
  {const auto led=h.ledImage(10);assert(led[unsigned(physical[0]-4)].known&&
     led[unsigned(physical[0]-4)].color==color);
   if(!eq)assert(led[unsigned(physical[1]-4)].color==MuteLedColor::Off&&
                 led[unsigned(physical[2]-4)].color==MuteLedColor::Off);}
  edge(h,physical[0],false,60);
  {const auto led=h.ledImage(60);assert(led[unsigned(physical[0]-4)].color==MuteLedColor::Off);}
  assert(h.desiredLive(cells[1]).value==64&&h.desiredLive(cells[2]).value==64);
  // Long on a different SPOT, peer resets after 120ms not at 119.
  edge(h,physical[1],true,100);flush(h,apis,value,1);
  h.advance(219,219*48);assert(!h.nextSourcePacket().valid);
  h.advance(220,220*48);flush(h,apis,value,2);
  for(int c:cells)assert(h.desiredLive(c).value==value);
  inspect(h,physical,color,220,true); // three LED flash together for 100ms
  edge(h,physical[1],false,240);assert(!h.nextSourcePacket().valid);
  inspect(h,physical,color,319,true);
  inspect(h,physical,color,321,false);
  // No timer case: physical true UP exactly at 120ms must qualify group.
  for(int c:cells)image.desired[unsigned(c)]=64;
  HostEngine boundary;assert(boundary.setNavForDiagnostic(nav)&&boundary.restoreSessionBeforeRendering(image));
  edge(boundary,physical[0],true,300);flush(boundary,apis,value,1);
  edge(boundary,physical[0],false,420);flush(boundary,apis,value,2);
  for(int c:cells)assert(boundary.desiredLive(c).value==value);
  inspect(boundary,physical,color,420,true);
  // STOP cancels the pending hold and its temporary light, not a fake UP.
  HostEngine stopped;assert(stopped.setNavForDiagnostic(nav)&&stopped.restoreSessionBeforeRendering(image));
  edge(stopped,physical[0],true,500);flush(stopped,apis,value,1);
  stopped.stop(501*48);
  stopped.advance(700,700*48);edge(stopped,physical[0],false,701);
  assert(!stopped.nextSourcePacket().valid);
  {const auto led=stopped.ledImage(701);assert(led[unsigned(physical[0]-4)].color==MuteLedColor::Off);}
 }
 std::cout<<"PASS SOC-03 INST/YELLOW/DRUMS/FILL/EQ short/120ms group, 100ms LED, exact UP, STOP\n";
}
