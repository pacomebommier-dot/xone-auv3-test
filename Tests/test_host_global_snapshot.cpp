#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static auto session(int value){xone::l01::LiveState::SessionImage s{};s.schema=1;s.config=1;s.revision=1;s.desired.fill(-2);s.desired[1328]=int16_t(value);s.desired[0]=22;s.desired[28]=4;return s;}
static GridBatch edge(GridKind kind,int ordinal){GridBatch b{};b.valid=true;b.count=1;b.events[0]={kind,0,ordinal};return b;}
static void cc(HostEngine& h,int value,int ms){uint8_t raw[]={0xbe,16,uint8_t(value)};auto r=h.receive(0,raw,3,ms,ms);assert(r.emit);h.sent(r,0);}
static void button(HostEngine& e,int note,bool down,int ms){uint8_t midi[]={uint8_t(down?0x9e:0x8e),uint8_t(note),uint8_t(down?127:0)};assert(!e.receive(0,midi,3,ms,ms).emit);}
static void click(HostEngine& e,int note,int ms){button(e,note,true,ms);button(e,note,false,ms+1);}
int main(){
 // A full projection queue cannot block global restore: obsolete absolute
 // writes are replaced, never future musical requests or real OFF debts.
 {HostEngine e;assert(e.restoreSessionBeforeRendering(session(64)));
  for(unsigned source=0;source<4;++source)assert(e.requestSourceRecall(source,3));
  e.onMusicalGrid(edge(GridKind::Bar,4));assert(e.capturePreset(0));cc(e,64,1);cc(e,100,2);
  for(int ordinal=8;ordinal<=20;ordinal+=4){for(unsigned source=0;source<4;++source)assert(e.requestSourceRecall(source,3));e.onMusicalGrid(edge(GridKind::Bar,ordinal));}
  assert(e.requestPresetRecall(0));e.onMusicalGrid(edge(GridKind::Bar,24));assert(e.pendingPreset()==-1&&e.desiredLive(1328).value==64);
  unsigned count=0;while(e.nextSourcePacket().valid){auto p=e.nextSourcePacket();assert(p.api!=572&&p.api!=573&&p.api<931);e.ackSourcePacket(p,true,0);++count;}
  assert(count<500);
 }
 {CueLoadBar cue;assert(cue.requestQualified(0,3));cue.onBar();auto packet=cue.next();assert(packet.valid&&cue.acknowledge(packet,true));
  assert(cue.requestQualified(1,7));assert(cue.restoreSelections({{-1,-1,-1,-1}}));packet=cue.next();assert(packet.valid&&packet.value==0&&cue.acknowledge(packet,true));
  assert(!cue.next().valid&&cue.pendingSelection(1)==7);cue.onBar();packet=cue.next();assert(packet.valid&&packet.group==1&&packet.value==127);}

 for(unsigned slot=0;slot<4;++slot){HostEngine e;assert(e.restoreSessionBeforeRendering(session(64)));assert(e.showPresetPage());
  click(e,36+int(slot),1);assert(e.presetEngaged(slot)&&!e.presetMusicallyValid(slot));
  cc(e,64,3);cc(e,100,4);click(e,36+int(slot),5);assert(e.pendingPreset()==int(slot));e.onMusicalGrid(edge(GridKind::Bar,4));
  assert(e.desiredLive(1328).value==64&&e.presetMusicallyValid(slot));}
 {HostEngine e;assert(e.restoreSessionBeforeRendering(session(64)));click(e,12,1);assert(e.redActive());cc(e,64,3);cc(e,100,4);
  click(e,12,5);assert(!e.redActive()&&e.desiredLive(1328).value==64);
  button(e,12,true,10);cc(e,64,11);cc(e,100,12);button(e,12,false,210);assert(!e.redActive()&&e.desiredLive(1328).value==64);
  button(e,12,true,220);e.stop(221);button(e,12,false,500);assert(!e.redActive());}
 // Q12: continuous values do not invalidate. A newly exposed family does.
 {HostEngine e;assert(e.restoreSessionBeforeRendering(session(64)));assert(e.capturePreset(0));assert(e.requestPresetRecall(0));
  cc(e,64,1);cc(e,100,2);assert(e.pendingPreset()==0&&e.presetMusicallyValid(0));click(e,36,3);
  assert(e.pendingPreset()==-1&&!e.presetMusicallyValid(0));e.onMusicalGrid(edge(GridKind::Bar,4));assert(e.desiredLive(1328).value==100);
  GlobalSnapshot saved{};assert(e.readPreset(0,saved)&&saved.live.desired[28]==-2);}
 // Q11: PRESET may publish during LONG, its release restores only original NAV.
 {HostEngine e;assert(e.restoreSessionBeforeRendering(session(64)));xone::l01::NavS3 nav;assert(nav.initializeDefault());auto global=nav.active();global.at(292)=2;global.contextMode=2;
  assert(xone::l01::NavS3::regenerate(global)&&e.setNavForDiagnostic(global));assert(e.capturePreset(0));assert(e.setNavForDiagnostic(nav.active()));
  button(e,13,true,1);button(e,14,true,2);e.advance(202,202);assert(e.navMode()==3);
  assert(e.requestPresetRecall(0));e.onMusicalGrid(edge(GridKind::Bar,4));assert(e.navMode()==2);
  button(e,13,false,203);assert(e.navMode()==2);button(e,14,false,204);assert(e.navMode()==0&&e.desiredLive(1328).value==64);}
 // S8 modes are excluded even if altered between capture and restore.
 {HostEngine e;auto s=session(64);assert(e.restoreSessionBeforeRendering(s)&&e.capturePreset(0));s.desired[28]=1;s.desired[1328]=100;
  assert(e.restoreSessionBeforeRendering(s));assert(e.requestPresetRecall(0));e.onMusicalGrid(edge(GridKind::Bar,4));assert(e.desiredLive(28).value==1);}
 // RED cancels MUTE requests from either side of its capture, keeps source pending.
 for(bool before:{false,true}){HostEngine e;e.enableNativeMutePage(true);assert(e.restoreSessionBeforeRendering(session(64)));
  click(e,13,1);assert(e.mutePageVisible());if(before)click(e,36,3);assert(e.captureRed());if(!before)click(e,36,5);
  assert(e.muteGesturePending());assert(e.requestSourceRecall(1,3));assert(e.restoreRed());assert(!e.muteGesturePending());
  e.onMusicalGrid(edge(GridKind::Bar,4));assert(!e.globalMute().effectivelyMuted(0)&&e.desiredLive(404).known);}
 // INIT and PRESET share order but distinct quantification; no fifth bank.
 for(bool presetFirst:{false,true}){HostEngine e;assert(e.restoreSessionBeforeRendering(session(64))&&e.capturePreset(0));GlobalSnapshot init{};assert(e.readPreset(0,init));init.live.desired[1328]=20;
  assert(e.configureGlobalInit(init));if(presetFirst){assert(e.requestPresetRecall(0)&&e.requestGlobalInit());}else{assert(e.requestGlobalInit()&&e.requestPresetRecall(0));}
  e.onMusicalGrid(edge(GridKind::Bar,4));assert(e.desiredLive(1328).value==(presetFirst?20:64));assert(!e.readPreset(4,init));}

 HostEngine h;assert(h.restoreSessionBeforeRendering(session(64)));assert(h.capturePreset(0));cc(h,64,1);cc(h,100,2);
 assert(h.requestPresetRecall(0));assert(h.desiredLive(1328).value==100);h.onMusicalGrid(edge(GridKind::Beat,1));assert(h.desiredLive(1328).value==100);
 h.onMusicalGrid(edge(GridKind::Bar,4));assert(h.desiredLive(1328).value==64);assert(h.desiredLive(28).value==4);assert(!h.desiredLive(1).known);
 assert(h.presetEngaged(0)&&h.presetMusicallyValid(0));
 assert(h.captureRed());cc(h,64,10);cc(h,99,11);assert(h.restoreRed());assert(h.desiredLive(1328).value==64&&!h.redActive());
 assert(h.captureRed());h.stop(12);assert(!h.redActive()&&!h.restoreRed());
 std::cout<<"PASS global PRESET capture/BAR recall and immediate RED with UNKNOWN preserved\n";
}
