#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static xone::l01::NavImage page(int family=3,int selection=1){
 xone::l01::NavS3 nav;assert(nav.initializeDefault());auto n=nav.active();int root=family==4||family==6;
 n.at(291)=root;
 if(family<5){n.at(299+2*root)=selection<5;n.at(300+2*root)=selection>4;n.at(293+2*root)=selection<5?selection:1;n.at(294+2*root)=selection>4?selection:0;}
 else {n.at(303+root)=1;n.at(297+root)=selection;}
 assert(xone::l01::NavS3::regenerate(n));return n;
}
static void physical(HostEngine& h,int p,int ms){for(int note=24;note<40;++note)if(xone::l01::SurfaceInput::physicalControl(note)==p){
 const uint8_t down[]={0x9e,uint8_t(note),127},up[]={0x8e,uint8_t(note),0};h.receive(0,down,3,ms,ms);h.receive(0,up,3,ms+1,ms+1);return;}assert(false);}
static int memoryButton(const xone::l01::NavImage& n,int slot){for(int p=4;p<20;++p){auto r=n.route(p);if((r.address/1024==5||r.address/1024==8)&&r.position==slot)return p;}assert(false);return -1;}
static auto session(){xone::l01::LiveState::SessionImage s{};s.schema=1;s.config=7;s.revision=1;s.desired.fill(-2);
 for(int o=0;o<28;++o)for(int f=0;f<9;++f)s.desired[xone::l01::FxS6::addressFor(o,f) ]=f<6?50:0;
 return s;
}
static auto beat(){GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Beat,0,1};return b;}
int main(){
 {HostEngine e;assert(e.restoreSessionBeforeRendering(session()));assert(e.captureMemory(0,0));assert(e.requestMemoryRecall(0,0));
  auto replacement=session();replacement.desired[1552]=77;assert(e.requestSessionRestore(replacement));e.beginRenderBuffer();
  assert(e.pendingMemorySlot(0)==-1);e.onMusicalGrid(beat());assert(e.desiredLive(1552).value==77);}

 for(int family=3;family<=6;++family)for(int selection=1;selection<=8;++selection){
  if(family<5&&(selection==4||selection==8))continue;
  auto n=page(family,selection);xone::l01::NavS3 nav;assert(nav.initializeDefault());assert(nav.stage(n)&&nav.commit());
  unsigned found=0;for(int p=4;p<20;++p){int owner=-1,slot=-1;if(!HostEngine::deriveMemory(nav,p,owner,slot))continue;
   int expected=family<5?6*(family-3)+selection-1-(selection>4):12+8*(family-5)+selection-1;
   assert(owner==expected);++found;HostEngine e;assert(e.setNavForDiagnostic(n));assert(e.restoreSessionBeforeRendering(session()));
   physical(e,p,1);assert(e.memoryEngaged(owner,slot));xone::l01::BankS6::MemoryRecord stored{};assert(e.readMemory(owner,slot,stored)&&stored.valid&&stored.values[0]==50);
  }assert(found==unsigned(family<5?4:3));
 }
 for(int source=0;source<4;++source)for(int patch=0;patch<2;++patch){
  HostEngine e;auto s=session();for(unsigned g=0;g<4;++g)for(unsigned f=0;f<60;++f)s.desired[SourcePatchBank::cell(g,f)]=f%30<18?71:0;
  assert(e.restoreSessionBeforeRendering(s));assert(e.saveSourcePatch(source,0));
  for(int owner=0;owner<12;++owner){assert(e.captureMemory(owner,0));assert(e.requestMemoryRecall(owner,0));}
  assert(e.requestSourceRecall(source,patch?0:3));
  GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,4};e.onMusicalGrid(b);
  for(int owner=0;owner<12;++owner){xone::l01::BankS6::MemoryRecord saved{};assert(e.readMemory(owner,0,saved)&&saved.values[0]==50);
   assert(e.memoryEngaged(owner,0)==(owner/3!=source));assert(e.pendingMemorySlot(owner)==-1);
   assert(e.desiredLive(xone::l01::FxS6::addressFor(owner,0)).value==(owner/3==source?0:50));}
  e.onMusicalGrid(beat());assert(e.desiredLive(1552+24*source).value==0);
 }
 {HostEngine e;assert(e.restoreSessionBeforeRendering(session()));assert(e.captureMemory(0,0));assert(e.captureMemory(3,0));
  assert(e.requestSourceRecall(0,3));assert(e.requestMemoryRecall(0,0));assert(e.requestMemoryRecall(3,0));
  GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,4};e.onMusicalGrid(b);
  assert(!e.memoryEngaged(0,0)&&e.memoryEngaged(3,0));assert(e.desiredLive(1552).value==0&&e.desiredLive(1576).value==50);
 }
 {HostEngine e;assert(e.restoreSessionBeforeRendering(session()));assert(e.captureMemory(0,0));assert(e.captureMemory(0,1));assert(e.captureMemory(3,0));
  assert(e.requestMemoryRecall(0,0));assert(!e.requestMemoryRecall(0,3));assert(e.pendingMemorySlot(0)==0);
  assert(e.requestMemoryRecall(0,1));assert(e.pendingMemorySlot(0)==1);assert(e.requestMemoryRecall(3,0));
  e.stop(0);assert(e.pendingMemorySlot(0)==-1&&e.pendingMemorySlot(3)==-1);e.onMusicalGrid(beat());assert(!e.nextSourcePacket().valid);
  xone::l01::BankS6::MemoryRecord saved{};assert(e.readMemory(0,1,saved)&&saved.valid);
 }

 HostEngine h;auto n=page();assert(h.setNavForDiagnostic(n));assert(h.restoreSessionBeforeRendering(session()));
 int p=memoryButton(n,0);physical(h,p,1); // first inactive engagement captures, no recall
 int cc=-1;for(int j=36;j<56;++j)if(continuousTarget(n.route(j)).cell==1552)cc=j-36;assert(cc>=0);
 uint8_t raw[]={0xbe,uint8_t(cc),50};auto result=h.receive(0,raw,3,3,3);assert(result.emit);h.sent(result,0);
 raw[2]=99;result=h.receive(0,raw,3,4,4);assert(result.emit);h.sent(result,0);assert(h.desiredLive(1552).value==99);
 physical(h,p,5);assert(h.desiredLive(1552).value==99);h.onMusicalGrid(beat());assert(h.desiredLive(1552).value==50);
 raw[2]=50;result=h.receive(0,raw,3,10,10);h.sent(result,0);raw[2]=99;result=h.receive(0,raw,3,11,11);assert(result.emit);h.sent(result,0);
 h.stop(12);physical(h,p,13);assert(h.desiredLive(1552).value==50&&h.pendingMemorySlot(0)==-1);
 std::cout<<"PASS physical MEMORY capture then BEAT recall\n";
}
