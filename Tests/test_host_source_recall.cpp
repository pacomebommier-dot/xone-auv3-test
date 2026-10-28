#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static xone::l01::NavImage selected(int group){
 xone::l01::NavS3 nav;assert(nav.initializeDefault());auto n=nav.active();int root=group/2;bool high=group%2;
 n.at(291)=root;n.at(299+2*root)=high?0:1;n.at(300+2*root)=high?1:0;
 n.at(293+2*root)=high?1:4;n.at(294+2*root)=high?8:0;
 assert(xone::l01::NavS3::regenerate(n));return n;
}
static void press(HostEngine& h,int group,int slot,int ms=1){
 xone::l01::NavS3 nav;assert(nav.initializeDefault());assert(nav.stage(selected(group))&&nav.commit());
 for(int p=4;p<20;++p){unsigned g=0,i=0;if(!CueLoadBar::derive(nav,p,g,i)||int(i%4)!=slot)continue;
  for(int note=24;note<40;++note)if(xone::l01::SurfaceInput::physicalControl(note)==p){
   const uint8_t down[]={0x9e,uint8_t(note),127},up[]={0x8e,uint8_t(note),0};h.receive(0,down,3,ms,ms);h.receive(0,up,3,ms+1,ms+1);return;}}
 assert(false);
}
static xone::l01::LiveState::SessionImage session(int value){
 xone::l01::LiveState::SessionImage s{};s.schema=1;s.config=7;s.revision=1;s.desired.fill(-2);
 for(unsigned g=0;g<4;++g)for(unsigned f=0;f<60;++f)s.desired[SourcePatchBank::cell(g,f)]=f%30<18?value:(value%2);
 return s;
}
static GridBatch bar(){GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,17,4};return b;}
static int noteFor(int p){for(int n=24;n<40;++n)if(xone::l01::SurfaceInput::physicalControl(n)==p)return n;assert(false);return -1;}
static void click(HostEngine& h,int physical,int ms){uint8_t down[]={0x9e,uint8_t(noteFor(physical)),127},up[]={0x8e,uint8_t(noteFor(physical)),0};h.receive(0,down,3,ms,ms);h.receive(0,up,3,ms+1,ms+1);}
int main(){
 {HostEngine e;e.stop(0);assert(e.requestSourceRecall(0,3));assert(e.desiredLive(320).known&&e.desiredLive(320).value==0);}

 {HostEngine e;assert(e.restoreSessionBeforeRendering(session(51)));assert(e.saveSourcePatch(0,0));
  assert(e.restoreSessionBeforeRendering(session(62)));assert(e.saveSourcePatch(0,1));assert(e.restoreSessionBeforeRendering(session(70)));
  assert(e.requestSourceRecall(0,1));e.onMusicalGrid(bar());assert(e.desiredLive(320).value==62);
  assert(e.requestSourceRecall(0,0));auto next=bar();next.events[0].ordinal=8;e.onMusicalGrid(next);assert(e.desiredLive(320).value==51);
  assert(e.requestSourceRecall(0,3));next.events[0].ordinal=12;e.onMusicalGrid(next);assert(e.desiredLive(320).value==0);
  SourcePatchBank::Patch saved{};assert(e.readSourcePatch(0,0,saved)&&saved.values[0]==51);
 }
 {HostEngine e;assert(e.restoreSessionBeforeRendering(session(23)));assert(e.saveSourcePatch(0,0));
  assert(e.requestSourceRecall(0,0));e.onMusicalGrid(bar());
  while(e.nextSourcePacket().valid){auto p=e.nextSourcePacket();e.ackSourcePacket(p,true,17);}
  e.stop(18);assert(e.pendingStopOffs()==24);
  e.beginStopRetryPass(18);unsigned off=0;for(;;){auto p=e.nextStopPacket();if(!p.emit)break;assert((p.bytes[0]&0xf0)==0x80);e.sent(p,0);++off;}
  assert(off==24&&e.pendingStopOffs()==0);e.beginStopRetryPass(19);assert(!e.nextStopPacket().emit);
 }

 {HostEngine e;xone::l01::NavS3 nav;assert(nav.initializeDefault());int selection=-1;
  for(int p=4;p<20;++p)if(nav.active().route(p).address==2*1024+3*32+4)selection=p;
  assert(selection>=0);click(e,selection,1);press(e,0,3,3);assert(e.pendingCueLoad()==1);
  click(e,4,5);e.onMusicalGrid(bar());assert(e.desiredLive(320).known&&e.desiredLive(320).value==0);
 }
 {HostEngine e;assert(e.requestSourceRecall(0,3)&&e.requestSourceRecall(1,3));e.onMusicalGrid(bar());
  assert(e.desiredLive(320).known&&e.desiredLive(404).known&&!e.desiredLive(488).known);
  unsigned count=0;while(e.nextSourcePacket().valid){auto p=e.nextSourcePacket();e.ackSourcePacket(p,true,17);++count;}assert(count==194);
 }

 {HostEngine fresh;assert(fresh.setNavForDiagnostic(selected(0)));
  assert(!fresh.desiredLive(320).known);press(fresh,0,3);fresh.onMusicalGrid(bar());
  assert(fresh.desiredLive(320).known&&fresh.desiredLive(320).value==0);}

 {ContinuousPickup pickup;assert(pickup.consider(0,128,50,50,1)==50);
  assert(pickup.consider(0,128,50,100,2)==100);pickup.rearm(128);
  assert(pickup.consider(0,128,50,100,3)==-1);}

 // Save/replay all twelve real local slots. Never imports SAMPLE or a fourth PATCH.
 for(unsigned g=0;g<4;++g)for(unsigned slot=0;slot<3;++slot){
  HostEngine h;assert(h.setNavForDiagnostic(selected(g)));assert(h.restoreSessionBeforeRendering(session(23+slot)));
  assert(h.saveSourcePatch(g,slot));SourcePatchBank::Patch stored{};assert(h.readSourcePatch(g,slot,stored));
  assert(!h.readSourcePatch(g,3,stored));assert(h.restoreSessionBeforeRendering(session(70)));
  press(h,g,slot);assert(h.desiredLive(SourcePatchBank::cell(g,0)).value==70);h.onMusicalGrid(bar());
  assert(h.sourceRecallStatus(g)==HostEngine::SourceRecallStatus::AppliedLocal);
  assert(h.desiredLive(SourcePatchBank::cell(g,0)).value==int(23+slot));
  for(unsigned other=0;other<4;++other)if(other!=g)assert(h.desiredLive(SourcePatchBank::cell(other,0)).value==70);
  auto packet=h.nextSourcePacket();assert(packet.valid&&packet.api==128+36*int(g)&&packet.value==int(23+slot));
  h.ackSourcePacket(packet,false,17);assert(!h.nextSourcePacket().valid);h.beginSourceRetryPass();
  auto retry=h.nextSourcePacket();assert(retry.token==packet.token&&retry.bytes==packet.bytes);
  h.ackSourcePacket(retry,true,18);assert(h.nextSourcePacket().token!=retry.token);
  assert(h.readSourcePatch(g,slot,stored)&&stored.values[0]==int(23+slot));
 }
 {HostEngine h;assert(h.setNavForDiagnostic(selected(0)));assert(h.restoreSessionBeforeRendering(session(70)));
  press(h,0,0);h.onMusicalGrid(bar());assert(h.sourceRecallStatus(0)==HostEngine::SourceRecallStatus::EmptyPatch);
  assert(h.desiredLive(320).value==70&&!h.nextSourcePacket().valid);}
 {HostEngine h;assert(h.setNavForDiagnostic(selected(0)));assert(h.restoreSessionBeforeRendering(session(70)));
  press(h,0,3);h.stop(10);h.onMusicalGrid(bar());assert(h.desiredLive(320).value==70&&!h.nextSourcePacket().valid);}
 {HostEngine h;assert(!h.saveSourcePatch(0,0));SourcePatchBank::Patch p{};assert(!h.readSourcePatch(0,0,p));}

 for(int g=0;g<4;++g){HostEngine h;assert(h.setNavForDiagnostic(selected(g)));
 xone::l01::LiveState::SessionImage s{};s.schema=1;s.config=7;s.revision=1;s.desired.fill(-2);s.desired[1328+g]=91;
 assert(h.restoreSessionBeforeRendering(s));press(h,g,3);assert(!h.desiredLive(320+84*g).known);
 GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,4};h.onMusicalGrid(b);
 assert(h.desiredLive(320+84*g).known&&h.desiredLive(320+84*g).value==0);
 assert(h.desiredLive(1552+24*g).known&&h.desiredLive(1552+24*g).value==0);
 assert(h.desiredLive(1768+12*g).known&&h.desiredLive(1768+12*g).value==0);
 assert(h.desiredLive(1546+g).known&&h.desiredLive(1546+g).value==0);
 assert(!h.desiredLive(1744).known&& !h.desiredLive(1351+g).known);
 assert(h.desiredLive(1328+g).value==91);assert(!h.desiredLive(320+84*g+30).known);
 }
 std::cout<<"PASS four physical source BASE recalls at BAR preserve MIX and S8 modes\n";
}

