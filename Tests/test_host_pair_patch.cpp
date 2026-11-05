#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void edge(HostEngine& e,int note,bool down,int ms){const uint8_t midi[]={uint8_t(down?0x9e:0x8e),uint8_t(note),uint8_t(down?127:0)};assert(!e.receive(0,midi,3,ms,ms).emit);}
static void setup(HostEngine& e,int selected,bool complete=true){
 xone::l01::NavS3 nav;assert(nav.initializeDefault());auto n=nav.active();n.at(292)=7;n.contextMode=7;n.at(314)=selected;assert(xone::l01::NavS3::regenerate(n)&&e.setNavForDiagnostic(n));
 xone::l01::LiveState::SessionImage s{};s.schema=1;s.config=5;s.revision=1;s.desired.fill(-2);
 unsigned source=unsigned(selected-1)/2;
 for(unsigned f=0;f<60;++f)s.desired[SourcePatchBank::cell(source,f)]=f%30<18?20+int(f/30):int(f%2);
 if(!complete)s.desired[SourcePatchBank::cell(source,selected%2?42:1)]=-2;
 assert(e.restoreSessionBeforeRendering(s));
}
int main(){
 for(int selected=1;selected<=8;++selected)for(int slot=0;slot<3;++slot){
  HostEngine e;setup(e,selected);unsigned source=unsigned(selected-1)/2;
  edge(e,14,true,1);edge(e,52+slot,true,800);edge(e,52+slot,false,801);edge(e,14,false,802);
  SourcePatchBank::Patch patch{};assert(e.readSourcePatch(source,slot,patch)&&patch.valid&&patch.values[0]==20&&patch.values[42]==21&&patch.values[30]==-2);
  assert(e.sampleSavesAccepted()==1&&e.sampleSavesRefused()==0&&e.navMode()==7);
  for(unsigned other=0;other<4;++other)if(other!=source)assert(!e.readSourcePatch(other,slot,patch));
  edge(e,52+slot,true,803);edge(e,52+slot,false,804);
  GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,4};e.onMusicalGrid(b);
  assert(e.sourceRecallStatus(source)==HostEngine::SourceRecallStatus::AppliedLocal);
  unsigned count=0;while(e.nextSourcePacket().valid){auto p=e.nextSourcePacket();e.ackSourcePacket(p,true,1);++count;}assert(count==37);
  assert(!e.desiredLive(320+84*int(source)+30).known&&!e.desiredLive(320+84*int(source)+72).known);
 }
 for(int selected=1;selected<=8;++selected){HostEngine e;setup(e,selected,false);edge(e,14,true,1);edge(e,52,true,2);edge(e,52,false,3);edge(e,14,false,4);
  SourcePatchBank::Patch p{};assert(e.sampleSavesRefused()==1&&!e.readSourcePatch(unsigned(selected-1)/2,0,p));assert(!e.samplePatchPending());}
 {
  HostEngine e;setup(e,1);
  SourcePatchBank::Patch patch{};
  edge(e,52,true,10);
  assert(!e.readSourcePatch(0,0,patch));
  edge(e,52,false,11);
  assert(e.readSourcePatch(0,0,patch)&&patch.revision==1);
  edge(e,52,true,12);edge(e,52,false,212);
  assert(e.readSourcePatch(0,0,patch)&&patch.revision==2&&!e.samplePatchPending());
  xone::l01::NavS3 nav;assert(nav.initializeDefault());
  auto cue=nav.active();cue.at(299)=1;cue.at(293)=4;
  assert(xone::l01::NavS3::regenerate(cue)&&nav.stage(cue)&&nav.commit());
  int note=-1;
  for(int n=24;n<40;++n){
   unsigned group=0,index=0;
   if(CueLoadBar::derive(nav,xone::l01::SurfaceInput::physicalControl(n),group,index)&&group==0&&index==0)note=n;
  }
  assert(note>=0);
  e.endRenderBuffer();
  auto document=e.documentForHost();
  document.current.nav=cue;
  document.live.desired[320]=47;
  document.current.live.desired[320]=47;
  HostEngine reopened;assert(reopened.requestDocumentRestore(document));
  reopened.beginRenderBuffer();
  edge(reopened,note,true,20);
  assert(reopened.pendingCueLoad()==0);
  edge(reopened,note,false,220);
  assert(reopened.readSourcePatch(0,0,patch)&&patch.revision==3&&patch.values[0]==47&&reopened.pendingCueLoad()==0);
  edge(reopened,note,true,230);edge(reopened,note,false,231);
  assert(reopened.pendingCueLoad()==1&&reopened.sourceRecallStatus(0)==HostEngine::SourceRecallStatus::Pending);
  GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,4};reopened.onMusicalGrid(b);
  assert(reopened.sourceRecallStatus(0)==HostEngine::SourceRecallStatus::AppliedLocal);
  edge(reopened,note,true,240);reopened.stop(241);edge(reopened,note,false,440);
  assert(reopened.readSourcePatch(0,0,patch)&&patch.revision==3);
 }
 std::cout<<"PASS shared pair PATCH SAVE36 from either MACHINE page, 24 physical slots, UNKNOWN partner rejection and TOP BAR recall\n";
}
