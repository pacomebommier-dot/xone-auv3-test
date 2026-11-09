#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static SourcePatchBank::Values base(int value){SourcePatchBank::Values v{};for(unsigned f=0;f<v.size();++f)v[f]=f%30<18?value:0;return v;}
static void bar(HostEngine& h,int n){GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,n};h.onMusicalGrid(b);}
static void flush(HostEngine& h){h.beginSourceRetryPass();for(unsigned i=0;i<4096;++i){auto p=h.nextSourcePacket();if(!p.valid)return;h.ackSourcePacket(p,true,0);}assert(false);}
int main(){
 for(unsigned source=0;source<4;++source)for(unsigned slot=0;slot<3;++slot)for(bool legacy:{false,true}){
  HostEngine h;
  if(legacy){SessionImage s{};s.schema=1;s.config=1;s.revision=1;s.desired.fill(-2);
   const auto v=base(99);for(unsigned f=0;f<60;++f)s.desired[SourcePatchBank::cell(source,f)]=static_cast<int16_t>(v[f]);
   assert(h.restoreSessionBeforeRendering(s)&&h.saveSourcePatch(source,slot));}
  assert(h.configureAlternativeBase(source/2,0,base(11),1)&&h.bindAlternativeSlot(source,0,0));
  assert(h.requestAlternative(source,0)==AlternativeArbiter::Request::Accepted);bar(h,4);flush(h);
  assert(h.activeAlternative(source)==0&&h.saveSourcePatch(source,slot));
  SourcePatchBank::Patch saved{};assert(h.readSourcePatch(source,slot,saved)&&saved.values[0]==11);
  assert(h.configureAlternativeBase(source/2,0,base(22),1));assert(h.requestSourceRecall(source,3));bar(h,8);flush(h);
  assert(h.desiredLive(SourcePatchBank::cell(source,0)).value==22);
  assert(h.requestSourceRecall(source,slot));bar(h,12);
  assert(h.sourceRecallStatus(source)==HostEngine::SourceRecallStatus::AppliedLocal);
  assert(h.desiredLive(SourcePatchBank::cell(source,0)).value==11);
  const auto packet=h.nextSourcePacket();assert(packet.valid&&packet.api==SourcePatchBank::api(source,0)&&packet.value==11);flush(h);
  assert(h.readSourcePatch(source,slot,saved)&&saved.values[0]==11);
 }
 std::cout<<"PASS active-alternative PATCH recalls use the same owned bank as SAVE across four sources/three slots, with and without a different legacy PATCH\n";
}
