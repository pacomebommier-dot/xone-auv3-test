#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <vector>
#include <iostream>
using namespace xone::host;
static auto flush(MuteLedOutput& out){std::vector<MuteLedOutput::Packet> packets;unsigned cursor=0;for(unsigned i=0;i<136;++i){auto p=out.next(cursor);if(!p.valid)break;packets.push_back(p);out.confirm(p,true);}return packets;}
static auto session(){SessionImage s{};s.schema=1;s.config=1;s.revision=1;s.desired.fill(-2);return s;}
static auto nav(int mode){xone::l01::NavS3 n;assert(n.initializeDefault());auto image=n.active();image.at(292)=mode;image.contextMode=mode;assert(xone::l01::NavS3::regenerate(image));return image;}
int main(){
 MuteLedOutput out;MuteLedOutput::Image image{};
 for(unsigned i=0;i<34;++i)image[i]={i==32?MuteLedColor::Red:static_cast<MuteLedColor>(1+i%3),false,true};
 out.observeFull(image,0);auto tx=flush(out);assert(tx.size()==134);
 unsigned ons=0,offs=0;for(auto p:tx){const auto b=xone::l01::ledBinding(static_cast<int>(p.target),static_cast<int>(p.color+1));assert(b.note>=0&&p.bytes[1]==b.note&&(p.bytes[0]&15)==14);if(p.on)++ons;else ++offs;}
 assert(ons==34&&offs==100&&flush(out).empty());
 image[0].blinking=true;out.observeFull(image,100);tx=flush(out);assert(tx.size()==1&&!tx[0].on);out.observeFull(image,200);tx=flush(out);assert(tx.size()==1&&tx[0].on);
 image[0].known=false;out.observeFull(image,201);assert(flush(out).size()==1); // UNKNOWN dark, not a fabricated musical OFF
 out.flash(16,MuteLedColor::Green,210);out.observeFull(image,210);tx=flush(out);assert(tx.size()==2&&tx.back().bytes[1]==120);out.observeFull(image,310);assert(flush(out).size()==2);
 out.flash(16,MuteLedColor::Green,320);out.cancelFlashes();out.observeFull(image,320);assert(flush(out).empty());
 image[33].color=MuteLedColor::Green;out.observeFull(image,400);unsigned cursor=0;auto failed=out.next(cursor);assert(failed.valid&&failed.target==33);out.confirm(failed,false);assert(flush(out).empty());
 out.beginBuffer();tx=flush(out);assert(tx.size()==4&&!tx[0].on&&tx.back().on&&tx.back().bytes[1]==23);
 HostEngine preset;assert(preset.showPresetPage());auto view=preset.ledImage();for(unsigned i=0;i<4;++i)assert(view[i].known&&view[i].color==MuteLedColor::Green);
 assert(!view[12].known&&view[32].known&&view[32].color==MuteLedColor::Off);
 assert(preset.capturePreset(0));assert(preset.ledImage()[0].color==MuteLedColor::Yellow);assert(preset.requestPresetRecall(0));assert(preset.ledImage()[0].blinking);
 GridBatch bar{};bar.valid=true;bar.count=1;bar.events[0]={GridKind::Bar,0,4};preset.onMusicalGrid(bar);assert(preset.ledImage()[0].color==MuteLedColor::Red&&!preset.ledImage()[0].blinking);
 preset.stop(0);assert(!preset.ledImage()[0].blinking);assert(preset.captureRed());assert(preset.ledImage()[32].color==MuteLedColor::Red);preset.stop(1);assert(preset.ledImage()[32].color==MuteLedColor::Off);
 SourcePatchBank::Values values{};for(unsigned f=0;f<60;++f)values[f]=f%30<18?42:0;
 HostEngine patch;auto live=session();for(unsigned f=0;f<60;++f)live.desired[static_cast<unsigned>(SourcePatchBank::cell(0,f))]=static_cast<int16_t>(values[f]);
 auto machine=nav(7);machine.at(314)=1;assert(xone::l01::NavS3::regenerate(machine));assert(patch.setNavForDiagnostic(machine)&&patch.restoreSessionBeforeRendering(live));
 assert(patch.ledImage()[28].color==MuteLedColor::Green);assert(patch.saveSourcePatch(0,0));assert(patch.ledImage()[28].color==MuteLedColor::Red);
 live.desired[320]=43;assert(patch.restoreSessionBeforeRendering(live));assert(patch.ledImage()[28].color==MuteLedColor::Yellow);
 live.desired[320]=-2;assert(patch.restoreSessionBeforeRendering(live));assert(!patch.ledImage()[28].known);
 HostEngine alternatives;assert(alternatives.configureAlternativeBase(0,0,values,1)&&alternatives.bindAlternativeSlot(0,0,0));assert(alternatives.showPresetPage()&&alternatives.consultPresetInstrumentGroup(0));
 assert(alternatives.requestAlternative(0,0)==AlternativeArbiter::Request::Accepted);view=alternatives.ledImage();assert(view[12].color==MuteLedColor::Green&&view[12].blinking);
 alternatives.stop(2);assert(!alternatives.ledImage()[12].blinking);
 // Every MEMORY owner is derived from the visible route; no hidden page steals the LED.
 for(int family=3;family<=6;++family)for(int page=1;page<=8;++page){if(family<5&&(page==4||page==8))continue;
  auto n=nav(0);const int root=family==4||family==6;n.at(291)=root;
  if(family<5){n.at(299+2*root)=page<5;n.at(300+2*root)=page>4;n.at(293+2*root)=page<5?page:1;n.at(294+2*root)=page>4?page:0;}
  else {n.at(303+root)=1;n.at(297+root)=page;}
  assert(xone::l01::NavS3::regenerate(n));xone::l01::NavS3 ns;assert(ns.initializeDefault()&&ns.stage(n)&&ns.commit());
  HostEngine memory;auto state=session();for(int owner=0;owner<28;++owner)for(int f=0;f<9;++f)state.desired[static_cast<unsigned>(xone::l01::FxS6::addressFor(owner,f))]=f<6?52:0;
  assert(memory.setNavForDiagnostic(n)&&memory.restoreSessionBeforeRendering(state));
  unsigned slots=0;for(int physical=4;physical<20;++physical){int owner=-1,slot=-1;if(!HostEngine::deriveMemory(ns,physical,owner,slot))continue;
   assert(memory.captureMemory(owner,slot));auto led=memory.ledImage()[static_cast<unsigned>(physical-4)];assert(led.known&&led.color==MuteLedColor::Red&&!led.blinking);
   assert(memory.requestMemoryRecall(owner,slot));led=memory.ledImage()[static_cast<unsigned>(physical-4)];assert(led.color==MuteLedColor::Green&&led.blinking);++slots;
  }assert(slots==static_cast<unsigned>(family<5?4:3));memory.stop(3);for(auto led:memory.ledImage())assert(!led.blinking);
 }
 // Physical pickup feedback can precede acquisition, but never fabricates LIVE.
 for(bool configured:{false,true}){HostEngine pickup;auto state=session();auto sample=nav(3);
  for(int f=0;f<28;++f)state.desired[static_cast<unsigned>(f)]=f<16?64:0;
  assert(pickup.setNavForDiagnostic(sample)&&pickup.restoreSessionBeforeRendering(state));
  if(configured)assert(pickup.setS8SampleModeBeforeRendering(0,0,2));
  pickup.beginRenderBuffer();pickup.prepareMuteLeds(0);unsigned c=0;for(unsigned i=0;i<136;++i){auto p=pickup.nextMuteLedPacket(c);if(!p.valid)break;pickup.ackMuteLedPacket(p,true);}
  const uint8_t cc[]{0xBE,4,0};assert(!pickup.receive(0,cc,3,10,10).emit);pickup.prepareMuteLeds(10);c=0;auto feedback=pickup.nextMuteLedPacket(c);
  if(configured){assert(feedback.valid&&feedback.on&&feedback.target==16&&feedback.bytes[1]==120);pickup.ackMuteLedPacket(feedback,true);pickup.prepareMuteLeds(110);c=0;auto off=pickup.nextMuteLedPacket(c);assert(off.valid&&!off.on&&off.bytes[1]==120);pickup.ackMuteLedPacket(off,true);}
  else assert(!feedback.valid);
  pickup.requestLedRefresh();pickup.beginRenderBuffer();pickup.prepareMuteLeds(120);c=0;unsigned purged=0;
  for(unsigned i=0;i<136;++i){auto p=pickup.nextMuteLedPacket(c);if(!p.valid)break;if(!p.on)++purged;pickup.ackMuteLedPacket(p,true);}assert(purged==100);
 }
 // Physical CUE/LOAD remains pending through a failed callback; green is confirmed selection only.
 {HostEngine cue;auto n=nav(0);n.at(293)=4;n.at(299)=1;assert(xone::l01::NavS3::regenerate(n)&&cue.setNavForDiagnostic(n));
  int control=-1,note=-1;for(int p=4;p<20;++p){auto route=n.route(p);if(route.address/1024==6&&route.position==3)control=p;}
  for(int key=24;key<40;++key){if(xone::l01::SurfaceInput::physicalControl(key)==control)note=key;}
  assert(note>=0);
  const uint8_t down[]{0x9E,static_cast<uint8_t>(note),127},up[]{0x8E,static_cast<uint8_t>(note),0};cue.receive(0,down,3,1,1);cue.receive(0,up,3,2,2);
  assert(cue.ledImage()[static_cast<unsigned>(control-4)].color==MuteLedColor::Yellow&&cue.ledImage()[static_cast<unsigned>(control-4)].blinking);
  cue.onMusicalGrid(bar);auto packet=cue.nextCueLoadPacket();assert(packet.valid);cue.ackCueLoadPacket(packet,false,3);assert(cue.ledImage()[static_cast<unsigned>(control-4)].blinking);
  cue.beginCueLoadRetryPass();packet=cue.nextCueLoadPacket();cue.ackCueLoadPacket(packet,true,4);assert(cue.ledImage()[static_cast<unsigned>(control-4)].color==MuteLedColor::Green&&!cue.ledImage()[static_cast<unsigned>(control-4)].blinking);
  cue.endRenderBuffer();HostEngine reopened;assert(reopened.requestDocumentRestore(cue.documentForHost()));reopened.beginRenderBuffer();
  assert(!reopened.ledImage()[static_cast<unsigned>(control-4)].known&&!reopened.nextCueLoadPacket().valid);
 }
 std::cout<<"PASS 34 LED bindings, UNKNOWN, blink/flash/retry, PRESET RED PATCH and alternative ownership\n";
}
