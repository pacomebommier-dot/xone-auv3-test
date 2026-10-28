#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
using A=AlternativeArbiter;
static SourcePatchBank::Values base(int value){SourcePatchBank::Values v{};for(unsigned f=0;f<60;++f){v[f]=f%30<18?value:0;}return v;}
static GridBatch bar(int n){GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,n};return b;}
static void flush(HostEngine& e){e.beginSourceRetryPass();for(unsigned i=0;i<4096;++i){auto p=e.nextSourcePacket();if(!p.valid)return;e.ackSourcePacket(p,true,0);}assert(false);}
static void note(HostEngine& e,int number,bool down,int ms){uint8_t raw[]={uint8_t(down?0x9e:0x8e),uint8_t(number),uint8_t(down?127:0)};assert(!e.receive(0,raw,3,ms,ms).emit);}
static void click(HostEngine& e,int number,int ms){note(e,number,true,ms);note(e,number,false,ms+1);}
static void machinePage(HostEngine& e){xone::l01::NavS3 nav;assert(nav.initializeDefault());auto n=nav.active();n.at(292)=7;n.contextMode=7;n.at(314)=1;assert(xone::l01::NavS3::regenerate(n)&&e.setNavForDiagnostic(n));}
static void machineCC(HostEngine& e,int value,int ms){uint8_t raw[]={0xbe,4,uint8_t(value)};auto p=e.receive(0,raw,3,ms,ms);if(p.emit)e.sent(p,0);assert(e.desiredLive(320).known&&e.desiredLive(320).value==value);}
int main(){
 // A global recall supersedes older selector intentions, but does not erase
 // a later independent alternative request admitted for the same BAR.
 {HostEngine h;assert(h.configureAlternativeBase(0,0,base(11),1)&&h.bindAlternativeSlot(0,0,0));
  assert(h.configureAlternativeBase(1,0,base(33),1)&&h.bindAlternativeSlot(2,0,0));assert(h.capturePreset(0));
  assert(h.requestAlternative(0,0)==A::Request::Accepted);assert(h.requestPresetRecall(0));assert(h.requestAlternative(2,0)==A::Request::Accepted);
  h.onMusicalGrid(bar(4));flush(h);assert(h.activeAlternative(0)==-1&&!h.desiredLive(320).known&&h.activeAlternative(2)==0&&h.desiredLive(488).value==33);}

 // Configurations stay with their instrument: UNKNOWN cannot inherit a
 // known legacy mode. A captured TOUCH closes on the old key before switch.
 {HostEngine h;machinePage(h);xone::l01::LiveState::SessionImage image{};image.schema=1;image.config=1;image.revision=1;image.desired.fill(-2);image.desired[350]=4;
  assert(h.restoreSessionBeforeRendering(image));assert(h.configureAlternativeBase(0,0,base(11),1)&&h.configureAlternativeBase(0,1,base(22),1));
  assert(h.bindAlternativeSlot(0,0,0)&&h.bindAlternativeSlot(0,1,1));assert(h.requestAlternative(0,0)==A::Request::Accepted);h.onMusicalGrid(bar(4));flush(h);
  uint8_t down[]={0x9e,48,127},up[]={0x8e,48,0};assert(!h.receive(0,down,3,1,1).emit);assert(!h.receive(0,up,3,2,2).emit);assert(h.desiredLive(338).value==0);
  assert(h.configureAlternativeMode(0,0,0,3));auto on=h.receive(0,down,3,3,3);assert(on.emit&&on.type==12&&on.api==96);h.sent(on,0);
  assert(h.requestAlternative(0,1)==A::Request::Accepted);h.onMusicalGrid(bar(8));auto off=h.nextSourcePacket();assert(off.valid&&off.type==13&&off.key==20&&h.activeAlternative(0)==0);
  assert(!h.receive(0,up,3,4,4).emit);h.ackSourcePacket(off,false,5);assert(h.activeAlternative(0)==0);flush(h);assert(h.activeAlternative(0)==1&&h.s8MachineMode(0,0)==-2);
  assert(!h.receive(0,down,3,6,6).emit);assert(!h.receive(0,up,3,7,7).emit);assert(h.desiredLive(338).value==0);
 }
 // Merely touching a source can preselect its editor group; no timer or
 // late release may retarget a selection captured on another group.
 {HostEngine h;uint8_t fader[]={0xbe,17,64};auto out=h.receive(0,fader,3,1,1);assert(out.emit);h.sent(out,0);assert(h.showPresetPage()&&h.presetInstrumentGroup()==1);
  assert(h.consultPresetInstrumentGroup(0));note(h,29,true,2);h.advance(203,203);note(h,29,false,204);assert(h.presetInstrumentGroup()==0);}

 // PRESET/RED restore captured values and IDs, never manual BASE afterwards.
 {HostEngine h;machinePage(h);assert(h.configureAlternativeBase(0,0,base(11),1)&&h.configureAlternativeBase(0,1,base(22),1));
  assert(h.bindAlternativeSlot(0,0,0)&&h.bindAlternativeSlot(0,1,1));assert(h.configureAlternativeMode(0,0,0,3)&&h.configureAlternativeMode(0,1,0,2));
  assert(h.requestAlternative(0,0)==A::Request::Accepted);h.onMusicalGrid(bar(4));flush(h);machineCC(h,11,1);machineCC(h,37,2);
  assert(h.saveSourcePatch(0,1)&&h.capturePreset(0));assert(h.configureAlternativeMode(0,0,0,4));
  assert(h.requestAlternative(0,1)==A::Request::Accepted);h.onMusicalGrid(bar(8));flush(h);assert(h.s8MachineMode(0,0)==2);assert(h.saveSourcePatch(0,1));
  assert(h.requestPresetRecall(0));h.onMusicalGrid(bar(12));flush(h);assert(h.activeAlternative(0)==0&&h.desiredLive(320).value==37&&h.s8MachineMode(0,0)==4);
  SourcePatchBank::Patch patch{};assert(h.readSourcePatch(0,1,patch)&&patch.values[0]==37);
  assert(h.captureRed());assert(h.requestAlternative(0,1)==A::Request::Accepted);h.onMusicalGrid(bar(16));flush(h);assert(h.readSourcePatch(0,1,patch)&&patch.values[0]==22);
  assert(h.configureAlternativeMode(0,0,0,1));assert(h.restoreRed());flush(h);assert(h.activeAlternative(0)==0&&h.desiredLive(320).value==37&&h.s8MachineMode(0,0)==1);
  GlobalSnapshot invalid{};assert(h.readPreset(0,invalid));invalid.alternatives[1]=0;assert(!h.configureGlobalInit(invalid));assert(h.activeAlternative(0)==0&&h.activeAlternative(1)==-1);
 }
 // PRESET handoff waits for the old instrument OFF; STOP cannot resurrect it.
 for(bool stop:{false,true}){HostEngine h;machinePage(h);auto loud=base(22);loud[18]=1;
  assert(h.configureAlternativeBase(0,0,base(11),1)&&h.configureAlternativeBase(0,1,loud,1));assert(h.bindAlternativeSlot(0,0,0)&&h.bindAlternativeSlot(0,1,1));
  assert(h.requestAlternative(0,0)==A::Request::Accepted);h.onMusicalGrid(bar(4));flush(h);machineCC(h,11,1);machineCC(h,37,2);assert(h.capturePreset(0));
  assert(h.requestAlternative(0,1)==A::Request::Accepted);h.onMusicalGrid(bar(8));flush(h);assert(h.requestPresetRecall(0));h.onMusicalGrid(bar(12));
  assert(h.alternativeSwitchPending()&&h.pendingPreset()==0&&h.activeAlternative(0)==1&&h.desiredLive(320).value==22);
  auto off=h.nextSourcePacket();assert(off.valid&&off.api==96&&off.value==0);h.ackSourcePacket(off,false,3);
  uint8_t blocked[]={0xbe,4,77};assert(!h.receive(0,blocked,3,4,4).emit&&h.desiredLive(320).value==22);
  if(stop){h.stop(5);h.ackSourcePacket(off,true,6);h.onMusicalGrid(bar(16));assert(h.activeAlternative(0)==1&&h.pendingPreset()==-1&&h.desiredLive(320).value==22);}
  else {flush(h);assert(!h.alternativeSwitchPending()&&h.activeAlternative(0)==0&&h.desiredLive(320).value==37);}
 }
 // Same-arbitration first wins; a later claim steals only PENDING and pins BASE.
 {HostEngine h;assert(h.configureAlternativeBase(0,2,base(11),1)&&h.bindAlternativeSlot(0,0,2)&&h.bindAlternativeSlot(1,0,2));
  assert(h.requestAlternative(0,0,1)==A::Request::Accepted);assert(h.requestAlternative(1,0,1)==A::Request::Rejected);
  assert(h.requestAlternative(1,0,2)==A::Request::Accepted&&h.pendingAlternative(0)==-1);
  assert(h.configureAlternativeBase(0,2,base(99),1));h.onMusicalGrid(bar(4));flush(h);assert(h.activeAlternative(1)==2&&h.desiredLive(404).value==11);
  assert(h.requestAlternative(1,0)==A::Request::SamePending&&h.desiredLive(404).value==11);
 }

 // The two lower PRESET rows consult an explicit group, then select a bound ID.
 {HostEngine h;assert(h.configureAlternativeBase(0,2,base(11),1)&&h.configureAlternativeBase(0,1,base(22),1));
  assert(h.bindAlternativeSlot(0,0,2)&&h.bindAlternativeSlot(0,1,1));assert(h.showPresetPage());assert(h.presetInstrumentGroup()==-1);
  click(h,24,1);assert(h.pendingAlternative(0)==-1);click(h,28,3);assert(h.presetInstrumentGroup()==0);
  click(h,24,5);assert(h.pendingAlternative(0)==2&&h.activeAlternative(0)==-1);h.onMusicalGrid(bar(4));flush(h);assert(h.activeAlternative(0)==2);
  note(h,25,true,10);click(h,29,11);note(h,25,false,13);assert(h.presetInstrumentGroup()==1&&h.pendingAlternative(0)==1&&h.pendingAlternative(1)==-1);
  h.onMusicalGrid(bar(8));flush(h);assert(h.activeAlternative(0)==1);
 }
 // Failed old OFF delays publication of the new owner, and STOP cancels it.
 for(bool stop:{false,true}){HostEngine h;auto sounding=base(11);sounding[18]=1;
  assert(h.configureAlternativeBase(0,0,sounding,1)&&h.configureAlternativeBase(0,1,base(22),1));
  assert(h.bindAlternativeSlot(0,0,0)&&h.bindAlternativeSlot(0,1,1));assert(h.requestAlternative(0,0)==A::Request::Accepted);h.onMusicalGrid(bar(4));flush(h);
  assert(h.activeAlternative(0)==0&&h.desiredLive(320).value==11);assert(h.requestAlternative(0,1)==A::Request::Accepted);h.onMusicalGrid(bar(8));
  auto off=h.nextSourcePacket();assert(off.valid&&off.type==11&&off.api==96&&off.value==0);h.ackSourcePacket(off,false,1);
  assert(!h.nextSourcePacket().valid&&h.activeAlternative(0)==0&&h.desiredLive(320).value==11);
  if(stop){h.stop(2);h.ackSourcePacket(off,true,3);h.onMusicalGrid(bar(12));assert(h.activeAlternative(0)==0&&h.desiredLive(320).value==11);}
  else {flush(h);assert(h.activeAlternative(0)==1&&h.desiredLive(320).value==22);}
 }

 HostEngine e;assert(e.presetInstrumentGroup()==-1&&e.activeAlternative(0)==-1);
 assert(e.requestAlternative(0,0)==A::Request::Rejected);
 assert(e.configureAlternativeBase(0,2,base(11),1)&&e.bindAlternativeSlot(0,0,2));
 assert(e.configureAlternativeBase(0,1,base(22),1)&&e.bindAlternativeSlot(0,1,1));
 assert(e.requestAlternative(0,0)==A::Request::Accepted);assert(e.activeAlternative(0)==-1);e.onMusicalGrid(bar(4));flush(e);
 assert(e.activeAlternative(0)==2&&e.desiredLive(320).value==11);assert(e.saveSourcePatch(0,0));
 assert(e.requestAlternative(0,1)==A::Request::Accepted);e.onMusicalGrid(bar(8));flush(e);assert(e.activeAlternative(0)==1&&e.desiredLive(320).value==22);
 SourcePatchBank::Patch p{};assert(!e.readSourcePatch(0,0,p));
 assert(e.requestAlternative(0,0)==A::Request::Accepted);e.onMusicalGrid(bar(12));flush(e);assert(e.readSourcePatch(0,0,p)&&p.values[0]==11);
 assert(e.bindAlternativeSlot(1,0,2));assert(e.requestAlternative(1,0)==A::Request::Rejected);
 assert(e.configureAlternativeBase(1,2,base(33),1)&&e.bindAlternativeSlot(2,0,2));assert(e.requestAlternative(2,0)==A::Request::Accepted);
 e.onMusicalGrid(bar(16));flush(e);assert(e.activeAlternative(0)==2&&e.activeAlternative(2)==2);
 std::cout<<"PASS native alternative selection, configured BASE, independent pools and instrument-owned PATCH banks\n";
}
