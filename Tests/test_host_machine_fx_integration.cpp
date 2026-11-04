#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static Result raw(HostEngine& h,int note,bool down,int ms){const uint8_t b[]{uint8_t(down?0x9E:0x8E),uint8_t(note),uint8_t(down?127:0)};return h.receive(0,b,3,ms,ms);}
static int noteFor(int physical){for(int n=12;n<56;++n)if(xone::l01::SurfaceInput::physicalControl(n)==physical)return n;assert(false);return -1;}
static void key(HostEngine& h,int physical,bool down,int ms){assert(!raw(h,noteFor(physical),down,ms).emit);}
static void machine(HostEngine& h,int ms){raw(h,13,true,ms);raw(h,15,true,ms+1);raw(h,15,false,ms+2);raw(h,13,false,ms+3);assert(h.navMode()==7);}
static Result cc(HostEngine& h,int value,int ms){const uint8_t b[]{0xBE,4,uint8_t(value)};auto r=h.receive(0,b,3,ms,ms);if(r.emit){assert(r.packetCount()==1);h.sent(r,0);}return r;}
static void grid(HostEngine& h,GridKind kind,int ordinal){GridBatch b{};b.valid=true;b.count=1;b.events[0]={kind,0,ordinal};h.onMusicalGrid(b);}
static void flush(HostEngine& h){h.beginSourceRetryPass();for(unsigned i=0;i<4096;++i){auto p=h.nextSourcePacket();if(!p.valid)return;h.ackSourcePacket(p,true,0);}assert(false);}
int main(){
 for(unsigned source=0;source<4;++source){HostEngine h;xone::l01::NavS3 nav;assert(nav.initializeDefault());auto n=nav.active();const int root=source/2,page=1+4*(source%2),selected=2*(source+1);
  n.at(291)=root;n.at(314)=selected;if(page<5){n.at(293+2*root)=page;n.at(299+2*root)=1;}else{n.at(294+2*root)=page;n.at(300+2*root)=1;}
  assert(xone::l01::NavS3::regenerate(n)&&h.setNavForDiagnostic(n));SessionImage s{};s.schema=1;s.config=1;s.revision=1;s.desired.fill(-2);
  for(unsigned f=0;f<60;++f)if(SourcePatchBank::included(f))s.desired[SourcePatchBank::cell(source,f)]=f%30<18?11:0;
  for(unsigned f=0;f<24;++f)s.desired[1552+24*source+f]=64;
  for(unsigned f=0;f<12;++f)s.desired[1768+12*source+f]=0;
  assert(h.restoreSessionBeforeRendering(s));ResetDefaults defaults;defaults.config=1;assert(defaults.set(718+24*source,22)&&h.requestResetDefaults(defaults));h.beginRenderBuffer();
  machine(h,1);assert(cc(h,64,10).api==715+24*int(source));assert(cc(h,80,11).value==80);
  raw(h,13,true,20);raw(h,12,true,21);raw(h,12,false,22);raw(h,13,false,23);assert(h.navMode()==0);
  if(root){raw(h,15,true,24);raw(h,15,false,25);}
  auto moved=cc(h,90,30);assert(moved.emit&&moved.api==715+24*int(source)&&moved.value==90); // same pickup in native FX
  machine(h,40);moved=cc(h,100,50);assert(moved.emit&&moved.value==100&&h.desiredLive(1552+24*source).value==100);
  auto on=raw(h,48,true,60);assert(on.emit&&on.api==384+12*int(source));h.sent(on,0);
  int selectedKey=-1;for(int p=4;p<20;++p)if(h.routeForControl(p).address==2*1024+2*32+selected)selectedKey=p;assert(selectedKey>=0);
  key(h,selectedKey,true,70);h.advance(270,270);key(h,selectedKey,false,271);assert(h.machineFxBorrowed());
  auto off=raw(h,48,false,280);assert(off.emit&&off.api==on.api&&off.value==0);h.sent(off,0); // captured destination survives view change
  assert(!raw(h,48,true,300).emit);auto reset=h.nextSourcePacket();assert(reset.valid&&reset.api==718+24*int(source)&&reset.value==22);flush(h);assert(!raw(h,48,false,301).emit);
  const int owner=3*source;assert(h.captureMemory(owner,0));cc(h,22,310);assert(cc(h,80,311).emit);
  assert(h.requestMemoryRecall(owner,0));grid(h,GridKind::Beat,2);flush(h);assert(h.desiredLive(1555+24*source).value==22);
  assert(h.capturePreset(0));cc(h,22,320);cc(h,80,321);assert(h.requestPresetRecall(0));grid(h,GridKind::Bar,4);flush(h);
  assert(h.desiredLive(1555+24*source).value==22&&h.machineFxBorrowed());
  assert(h.captureRed());cc(h,22,330);cc(h,90,331);assert(h.restoreRed());flush(h);assert(h.desiredLive(1555+24*source).value==22);
  key(h,selectedKey,true,400);h.advance(600,600);key(h,selectedKey,false,601);assert(!h.machineFxBorrowed());
  on=raw(h,48,true,610);assert(on.emit);h.sent(on,0);assert(h.saveSourcePatch(source,1)&&h.requestSourceRecall(source,1));grid(h,GridKind::Bar,8);flush(h);
  assert(h.desiredLive(1768+12*source).value==1);off=raw(h,48,false,820);assert(off.emit&&off.api==on.api);h.sent(off,0);
  on=raw(h,48,true,830);assert(on.emit);h.sent(on,0);h.stop(840);off=h.nextStopPacket();assert(off.emit&&off.api==on.api);h.sent(off,0);
  assert(!h.nextStopPacket().emit&&!raw(h,48,false,850).emit);
 }
 std::cout<<"PASS MP03/04/05/06 four sources: native/alias shared pickup LIVE, held SPOT view change, borrowed RESET, MEMORY/PRESET/RED, PATCH exclusion and STOP\n";
}
