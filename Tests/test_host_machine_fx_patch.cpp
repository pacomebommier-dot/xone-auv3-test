#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
using namespace xone::host;
static void bar(HostEngine& h,int ordinal){GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,ordinal};h.onMusicalGrid(b);}
static unsigned flush(HostEngine& h){h.beginSourceRetryPass();unsigned n=0;for(;n<4096;++n){auto p=h.nextSourcePacket();if(!p.valid)return n;h.ackSourcePacket(p,true,0);}assert(false);return n;}
int main(){
 for(unsigned source=0;source<4;++source){HostEngine h;SessionImage s{};s.schema=1;s.config=1;s.revision=1;s.desired.fill(-2);
  for(unsigned f=0;f<60;++f)if(SourcePatchBank::included(f))s.desired[SourcePatchBank::cell(source,f)]=f%30<18?44:1;
  for(int f=0;f<24;++f)s.desired[1552+24*source+f]=64;
  for(int f=0;f<12;++f)s.desired[1768+12*source+f]=1;
  assert(h.restoreSessionBeforeRendering(s)&&h.saveSourcePatch(source,0));SourcePatchBank::Patch p{};assert(h.readSourcePatch(source,0,p));
  for(unsigned f=0;f<60;++f)assert(p.values[f]==(SourcePatchBank::included(f)?(f%30<18?44:1):-2));
  auto edit=s;for(unsigned f=0;f<60;++f)if(SourcePatchBank::included(f))edit.desired[SourcePatchBank::cell(source,f)]=f%30<18?22:0;
  for(int f=0;f<24;++f)edit.desired[1552+24*source+f]=77;
  assert(h.restoreSessionBeforeRendering(edit)&&h.requestSourceRecall(source,0));bar(h,4);
  assert(h.sourceRecallStatus(source)==HostEngine::SourceRecallStatus::AppliedLocal);
  assert(h.desiredLive(SourcePatchBank::cell(source,42)).value==44);
  assert(!h.desiredLive(SourcePatchBank::cell(source,30)).known);
  for(int f=0;f<24;++f)assert(h.desiredLive(1552+24*source+f).value==77);
  for(int f=0;f<12;++f)assert(h.desiredLive(1768+12*source+f).value==1);
  assert(flush(h)==37); // 36 MACHINE fields + existing source FILTER, no FX aliases
  assert(h.requestSourceRecall(source,3));bar(h,8);assert(flush(h)==73); // explicit BASE keeps its native FX domain
 }
 std::ifstream file("Fixtures/legacy_pair_patch_v2.bin",std::ios::binary);assert(file.good());
 std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});DocumentImage legacy{};
 assert(decodeDocument(bytes.data(),bytes.size(),legacy));SourcePatchBank::Patch old{};
 assert(legacy.source.read(0,0,old)&&old.values[30]==44&&old.values[48]==1); // opaque old fields preserved
 HostEngine restored;assert(restored.requestDocumentRestore(legacy));restored.beginRenderBuffer();restored.endRenderBuffer();flush(restored);
 assert(restored.requestSourceRecall(0,0));bar(restored,4);assert(flush(restored)==37);
 assert(!restored.desiredLive(1552).known); // old PATCH cannot overwrite native FX
 std::cout<<"PASS MP05 compatible PATCH domain: odd MACHINE30 + even POT/FADER6, no FX capture/replay, explicit BASE, real legacy V2 fixture\n";
}
