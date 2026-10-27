#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void edge(HostEngine& e,int n,bool down,int ms){const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(n),static_cast<uint8_t>(down?127:0)};e.receive(0,b,3,ms*48,ms);}
static void sample(HostEngine& e){edge(e,13,true,10);edge(e,14,true,20);edge(e,14,false,30);edge(e,13,false,40);assert(e.navMode()==3);}
static GridBatch grid(GridKind kind,int ordinal){GridBatch b{};b.valid=true;b.count=1;b.events[0]={kind,0,ordinal};return b;}
static xone::l01::LiveState::SessionImage session(){xone::l01::LiveState::SessionImage i{};i.schema=1;i.config=7;i.revision=1;i.desired.fill(-2);for(int j=0;j<40;++j)i.desired[j]=j<28?0:1;return i;}
static xone::l01::BankS6::PatchImage patch(){xone::l01::BankS6::PatchImage p{};p.fill(-2);for(int j=0;j<40;++j)p[j]=j<16?20+j:j<28?j%2:2;return p;}
int main(){
 HostEngine e;assert(e.restoreSessionBeforeRendering(session()));const auto saved=patch();
 assert(e.storeSamplePatchBeforeRendering(0,0,7,saved,0));sample(e);
 assert(!e.samplePatchOnGrid(grid(GridKind::Bar,4),100).emit);
 edge(e,52,true,50);edge(e,52,false,60);assert(e.samplePatchPending());
 assert(e.desiredLive(0).value==0);
 assert(!e.samplePatchOnGrid(grid(GridKind::Beat,5),200).emit&&e.samplePatchPending());
 assert(!e.samplePatchOnGrid(grid(GridKind::Bar,4),210).emit&&e.samplePatchPending());
 auto r=e.samplePatchOnGrid(grid(GridKind::Bar,8),300);assert(r.packetCount()==28&&!e.samplePatchPending());
 for(unsigned i=0;i<r.packetCount();++i){auto p=r.packet(i);assert(e.canSend(p));assert(p.sample==300);if(i<16){assert(p.type==10&&p.api==static_cast<int>(i)&&p.bytes[0]==0xB6&&p.bytes[1]==i&&p.bytes[2]==20+i);}else{assert(p.type==11&&p.api==static_cast<int>(i-16));assert(p.bytes[0]==((i-16)%2?0x96:0x86));}e.sent(p,0);}
 for(int j=0;j<40;++j)assert(e.desiredLive(j).known&&e.desiredLive(j).value==saved[j]);
 assert(!e.desiredLive(40).known&&!e.desiredLive(1476).known);
 assert(!e.samplePatchOnGrid(grid(GridKind::Bar,8),400).emit);
 edge(e,52,true,70);edge(e,52,false,80);assert(e.samplePatchPending());
 e.stop(500);assert(!e.samplePatchPending());assert(!e.samplePatchOnGrid(grid(GridKind::Bar,12),600).emit);
 HostEngine empty;sample(empty);edge(empty,52,true,50);edge(empty,52,false,60);assert(!empty.samplePatchPending()&&!empty.desiredLive(0).known);
 // Invalid data/profile/revision must not seed a usable bank or modify LIVE.
 {
  HostEngine invalid;assert(invalid.restoreSessionBeforeRendering(session()));auto bad=saved;bad[0]=-2;
  assert(!invalid.storeSamplePatchBeforeRendering(0,0,7,bad,0));
  assert(!invalid.storeSamplePatchBeforeRendering(0,0,8,saved,0));
  assert(!invalid.storeSamplePatchBeforeRendering(8,0,7,saved,0));
  assert(invalid.storeSamplePatchBeforeRendering(0,0,7,saved,0));
  assert(!invalid.storeSamplePatchBeforeRendering(0,0,7,saved,0));
  sample(invalid);assert(!invalid.storeSamplePatchBeforeRendering(0,1,7,saved,0));
 }
 // A queued recall keeps its original SAMPLE even after leaving its page.
 {
  HostEngine captured;assert(captured.restoreSessionBeforeRendering(session()));assert(captured.storeSamplePatchBeforeRendering(0,0,7,saved,0));sample(captured);
  edge(captured,52,true,50);edge(captured,52,false,60);edge(captured,15,true,70);edge(captured,15,false,80);assert(captured.navMode()==0);
  auto output=captured.samplePatchOnGrid(grid(GridKind::Bar,4),700);assert(output.emit&&captured.desiredLive(0).value==20);
  assert(!captured.desiredLive(40).known&&!captured.desiredLive(350).known);
 }
 // Failed MIDI ON still owes an OFF; STOP suppresses remaining stale ONs.
 {
  HostEngine failed;assert(failed.restoreSessionBeforeRendering(session()));assert(failed.storeSamplePatchBeforeRendering(0,0,7,saved,0));sample(failed);
  edge(failed,52,true,50);edge(failed,52,false,60);auto output=failed.samplePatchOnGrid(grid(GridKind::Bar,4),700);
  auto on=output.packet(17);assert(on.type==11&&on.value==1);failed.sent(on,-1);
  failed.stop(701);assert(!failed.canSend(on));auto off=failed.nextStopPacket();assert(off.emit&&off.api==1&&off.value==0);
  failed.sent(off,0);assert(!failed.nextStopPacket().emit);
 }
 // Physical TOUCH ownership is never stolen by PATCH, including its owed OFF.
 {
  HostEngine held;auto image=session();image.desired[28]=3;assert(held.restoreSessionBeforeRendering(image));assert(held.storeSamplePatchBeforeRendering(0,0,7,saved,0));sample(held);
  const uint8_t down[]{0x9E,48,127};auto touch=held.receive(0,down,3,50*48,50);assert(touch.emit&&touch.type==12);held.sent(touch,0);
  edge(held,52,true,60);edge(held,52,false,70);assert(held.samplePatchPending());
  assert(!held.samplePatchOnGrid(grid(GridKind::Bar,4),800).emit&&!held.samplePatchPending());assert(held.desiredLive(0).value==0&&held.desiredLive(28).value==3);
  held.stop(801);auto off=held.nextStopPacket();assert(off.emit&&off.type==13&&off.api==0);held.sent(off,0);
 }
 // A late field failure cannot publish a partially recalled LIVE image.
 {
  HostEngine limit;auto image=session();image.revision=xone::l01::LiveState::MaxRevision-10;
  assert(limit.restoreSessionBeforeRendering(image));assert(limit.storeSamplePatchBeforeRendering(0,0,7,saved,0));sample(limit);edge(limit,52,true,50);edge(limit,52,false,60);
  assert(!limit.samplePatchOnGrid(grid(GridKind::Bar,4),900).emit);
  for(int j=0;j<40;++j)assert(limit.desiredLive(j).value==image.desired[j]);
 }
 // Restoring a session cancels a recall captured in the preceding session.
 {
  HostEngine restored;assert(restored.restoreSessionBeforeRendering(session()));assert(restored.storeSamplePatchBeforeRendering(0,0,7,saved,0));sample(restored);
  edge(restored,52,true,50);edge(restored,52,false,60);assert(restored.samplePatchPending());
  assert(restored.requestSessionRestore(session()));restored.beginRenderBuffer();assert(!restored.samplePatchPending());
  assert(!restored.samplePatchOnGrid(grid(GridKind::Bar,4),1000).emit&&restored.desiredLive(0).value==0);
 }
 std::cout<<"PASS SAMPLE TOP PATCH -> real BAR -> 40 LIVE fields and 28 sourced MIDI packets, one-shot and STOP\n";
}
