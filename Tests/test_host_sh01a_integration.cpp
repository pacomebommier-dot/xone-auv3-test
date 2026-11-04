#include "../Shared/XoneHostEngine.hpp"
#include "../Shared/XoneSh01aHostBridge.hpp"
#include "../Shared/XoneSh01aProfile.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void bar(HostEngine& h){GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,4};h.onMusicalGrid(b);}
static void rx(HostEngine& h,int pc,int64_t t){const uint8_t b[]{0xC0,uint8_t(pc)};assert(!h.receive(0,b,2,t,t).emit);}
static void full(HostEngine& h,const Sh01aProfile& p,int value,int64_t t){for(auto cc:p.cc){const uint8_t b[]{0xB0,cc,uint8_t(value)};assert(!h.receive(0,b,3,t,t).emit);}}
int main(){
 // The AU exposes one output on cable0: S7 channels7..14 and K2 LED15
 // cannot also identify an independent SH destination by a channel filter.
 {HostEngine route;for(unsigned ch=0;ch<16;++ch){auto a=provisionalSh01aAttachment(0,1,-1,17,static_cast<uint8_t>(ch),0,1000);
  const bool dedicated=ch<6||ch==15;assert(a.valid()==dedicated);assert(route.requestSh01aAttachment(a)==dedicated);
  Sh01aAttachmentBytes wire{};assert(encodeSh01aAttachment(a,wire)==dedicated);
 }}

 for(unsigned source=0;source<4;++source){
 const unsigned otherSource=(source+1)%4;
 auto receipt=provisionalSh01aAttachment(0,1,-1,17,0,0,1000);
 assert(receipt.valid());
 for(unsigned i=0;i<36;++i){const int cc=receipt.image.profile.cc[i];
  assert(receipt.fields[i]==(cc==74?15:cc==71?16:cc==25?17:-1));}
 Sh01aAttachment a;a.enabled=true;a.source=static_cast<uint8_t>(source);a.image.profile=provisionalSh01aProfile(17,0,0,1000);
 unsigned cutoff=0;while(a.image.profile.cc[cutoff]!=74)++cutoff;a.fields[cutoff]=0;assert(a.valid());
 auto illegal=a;illegal.fields[0]=30;assert(!illegal.valid()); // even-page central FX excluded
 Sh01aAttachmentBytes wire{};assert(encodeSh01aAttachment(a,wire));Sh01aAttachment decoded;
 assert(decodeSh01aAttachment(wire.data(),wire.size(),decoded)&&decoded.image.profile==a.image.profile);
 HostEngine h;xone::l01::NavS3 nav;assert(nav.initializeDefault());auto n=nav.active();n.at(292)=7;n.contextMode=7;n.at(314)=static_cast<int>(2*source+1);
 assert(xone::l01::NavS3::regenerate(n)&&h.setNavForDiagnostic(n));
 assert(h.requestSh01aAttachment(a));h.beginRenderBuffer();h.endRenderBuffer();
 rx(h,0,1);full(h,a.image.profile,64,2);h.endRenderBuffer();auto saved=h.sh01aForHost();
 assert(saved.image.mirrors[0].valid&&h.desiredLive(SourcePatchBank::cell(source,0)).value==64&&!h.nextSh01aPacket().valid);
 assert(!h.saveSourcePatch(source,0));assert(!h.desiredLive(1552+24*static_cast<int>(source)).known); // SH never resets FX on PATCH
 h.observeTransport(false);assert(h.requestSourceRecall(otherSource,3));
 for(unsigned i=0;i<128;++i){auto p=h.nextSourcePacket();if(!p.valid)break;h.ackSourcePacket(p,true,2);}
 assert(h.saveSourcePatch(otherSource,2));SourcePatchBank::Patch other{};assert(h.readSourcePatch(otherSource,2,other)&&other.valid);
 h.observeTransport(true);
 assert(h.requestSourceRecall(source,1));assert(!h.nextSh01aPacket().valid);bar(h);
 auto pc=h.nextSh01aPacket();assert(pc.valid&&pc.length==2&&pc.bytes[0]==0xC0&&pc.bytes[1]==1);
 assert(!h.nextSourcePacket().valid);h.ackSh01aPacket(pc,false,3,3);assert(!h.nextSh01aPacket().valid);
 h.beginRenderBuffer();pc=h.nextSh01aPacket();assert(pc.valid&&pc.length==2);h.ackSh01aPacket(pc,true,4,4);
 assert(!h.nextSh01aPacket().valid&&!h.nextSourcePacket().valid);full(h,a.image.profile,37,5);h.endRenderBuffer();
 saved=h.sh01aForHost();assert(saved.image.mirrors[1].valid&&saved.image.mirrors[1].values[cutoff]==37);
 assert(h.ledImage()[29].known&&h.ledImage()[29].color==MuteLedColor::Red);
 const uint8_t move[]{0xBE,4,37};auto nominal=h.receive(0,move,3,6,6);if(nominal.emit)h.sent(nominal,0);
 auto edit=h.nextSh01aPacket();assert(edit.valid&&edit.length==3&&edit.bytes[0]==0xB0&&edit.bytes[1]==74&&edit.bytes[2]==37);
 h.ackSh01aPacket(edit,true,6,6);assert(!h.nextSh01aPacket().valid);h.endRenderBuffer();
 assert(h.sh01aForHost().image.mirrors[1].values[cutoff]==37);
 assert(h.capturePreset(0));const uint8_t external[]{0xB0,74,60};h.receive(0,external,3,7,7);
 assert(h.desiredLive(SourcePatchBank::cell(source,0)).value==60&&h.ledImage()[29].color==MuteLedColor::Yellow);h.observeTransport(false);assert(h.requestPresetRecall(0));
 assert(h.desiredLive(SourcePatchBank::cell(source,0)).value==60&&!h.nextSh01aPacket().valid); // PRESET never replays bound SH CC
 assert(h.captureRed());const uint8_t edit2[]{0xB0,74,61};h.receive(0,edit2,3,8,8);assert(h.restoreRed());
 assert(h.desiredLive(SourcePatchBank::cell(source,0)).value==61&&!h.nextSh01aPacket().valid);
 for(unsigned i=0;i<4096;++i){auto p=h.nextSourcePacket();if(!p.valid)break;h.ackSourcePacket(p,true,8);}
 rx(h,2,9);assert(h.ledImage()[30].blinking);const uint8_t partial[]{0xB0,74,99};h.receive(0,partial,3,10,10);h.stop(11);h.endRenderBuffer();
 assert(!h.ledImage()[30].known);
 saved=h.sh01aForHost();assert(!saved.image.mirrors[2].valid&&saved.image.mirrors[0].valid);
 HostEngine reopened;assert(reopened.requestSh01aAttachment(saved));reopened.beginRenderBuffer();reopened.endRenderBuffer();
 assert(reopened.sh01aForHost().image.mirrors[1].valid&&!reopened.nextSh01aPacket().valid);
 assert(reopened.sh01aLive(74)==-2); // persisted mirrors are not fresh observations
 }
 std::cout<<"PASS SH36 host across four sources configured route, S3 LIVE, SAVE guard, BAR PC-only, callback retry, physical CC, mirrors and no restore TX; hardware NOT_TESTED\n";
}
