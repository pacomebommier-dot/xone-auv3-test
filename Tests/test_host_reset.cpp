#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void edge(HostEngine& e,int note,bool down,int ms){
 const uint8_t d[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};
 assert(!e.receive(0,d,3,ms*48,ms).emit);
}
static void tap(HostEngine& e,int note,int ms){edge(e,note,true,ms);edge(e,note,false,ms+1);}
static void flush(HostEngine& e,bool success=true){
 for(unsigned n=0;n<2048;++n){auto p=e.nextSourcePacket();if(!p.valid)break;e.ackSourcePacket(p,success,0);if(!success)break;}
}
int main(){
 HostEngine e;
 // Numeric values here are explicit test configuration, never defaults inferred from boot.
 ResetDefaults defaults;defaults.config=1;
 assert(defaults.set(708,63)); // single FILTER GLOBAL/TOTAL identity, LIVE1476
 assert(defaults.set(907,41)); // borrowed/RESET INIT parameter, LIVE1744
 assert(!defaults.set(569,0)&&!defaults.set(572,0)&&!defaults.set(573,0)&&!defaults.set(931,0)&&!defaults.set(708,128));
 tap(e,55,1);assert(!e.desiredLive(1476).known&&!e.nextSourcePacket().valid);
 ResetDefaultsBytes bytes{};assert(encodeResetDefaults(defaults,bytes));ResetDefaults round;
 assert(decodeResetDefaults(bytes.data(),bytes.size(),round)&&round.values[708]==63&&round.config==1);
 auto bad=bytes;bad[16]=1;sessionPut32(bad.data()+bad.size()-4,sessionCrc(bad.data(),bad.size()-4));
 assert(!decodeResetDefaults(bad.data(),bad.size(),round)&&round.values[708]==63);
 assert(e.requestResetDefaults(defaults));e.beginRenderBuffer();
 assert(!e.desiredLive(1476).known&&!e.nextSourcePacket().valid); // configuration emits nothing
 tap(e,55,10);assert(e.desiredLive(1476).known&&e.desiredLive(1476).value==63);
 auto p=e.nextSourcePacket();assert(p.valid&&p.type==10&&p.api==708&&p.value==63);
 assert(p.bytes[0]==0xBB&&p.bytes[1]==68&&p.bytes[2]==63);
 e.ackSourcePacket(p,false,0);assert(!e.nextSourcePacket().valid);
 e.beginSourceRetryPass();assert(e.nextSourcePacket().valid);flush(e);
 // All scopes before mutation: an unknown member refuses the whole batch.
 std::array<ContinuousTarget,97> group{};group[0]={1476,708};group[1]={1744,907};group[2]={1745,908};
 assert(!e.resetContinuousTargets(group,3));
 assert(e.desiredLive(1476).value==63&&!e.desiredLive(1744).known&&!e.nextSourcePacket().valid);
 assert(e.resetContinuousTargets(group,2));assert(e.desiredLive(1744).value==41);flush(e);
 // No stale default from a different configuration, no partially accepted table.
 defaults.config=2;assert(e.requestResetDefaults(defaults));e.beginRenderBuffer();
 uint8_t cc[]{0xBE,3,63};auto seed=e.receive(0,cc,3,912,19);if(seed.emit)e.sent(seed,0);
 cc[2]=91;auto r=e.receive(0,cc,3,960,20);assert(r.emit);e.sent(r,0);
 tap(e,55,30);assert(e.desiredLive(1476).value==91&&!e.nextSourcePacket().valid);
 defaults.config=1;defaults.values[0]=200;assert(!e.requestResetDefaults(defaults));
 defaults.values[0]=-2;assert(e.requestResetDefaults(defaults));e.beginRenderBuffer();
 // TOP4 stays the same target through PRESET and a held TOP is cancelled by STOP.
 assert(e.showPresetPage());tap(e,55,40);assert(e.desiredLive(1476).value==63);flush(e);
 edge(e,55,true,50);e.stop(51*48);edge(e,55,false,52);assert(!e.nextSourcePacket().valid);
 // RESET-mode SPOT resolves its continuous identity, does not toggle an effect.
 HostEngine spot;ResetDefaults sp;sp.config=1;assert(sp.set(907,41));
 assert(spot.requestResetDefaults(sp));spot.beginRenderBuffer();
 edge(spot,14,true,10);edge(spot,15,true,20);edge(spot,15,false,30);edge(spot,14,false,40);
 assert(spot.navMode()==5);tap(spot,48,50);
 assert(spot.desiredLive(1744).known&&spot.desiredLive(1744).value==41);
 assert(!spot.desiredLive(1864).known);flush(spot);
 // Contract RESET SAMPLE: BASE short resets12 CC, long resets the four-member group.
 HostEngine sample;ResetDefaults sampleDefaults;sampleDefaults.config=1;
 for(unsigned voice=0;voice<4;++voice)for(unsigned q=0;q<12;++q)assert(sampleDefaults.set(16*voice+q,35+static_cast<int>(q)));
 assert(sample.requestResetDefaults(sampleDefaults));sample.beginRenderBuffer();
 edge(sample,13,true,10);edge(sample,14,true,20);edge(sample,14,false,30);edge(sample,13,false,40);
 assert(sample.navMode()==3);
 edge(sample,14,true,50);edge(sample,15,true,60);edge(sample,15,false,70);edge(sample,14,false,80);
 assert(sample.navMode()==5);
 tap(sample,36,90);assert(sample.desiredLive(0).known&&sample.desiredLive(0).value==35);
 assert(!sample.desiredLive(16).known&&!sample.desiredLive(28).known&&!sample.desiredLive(40).known);flush(sample);
 edge(sample,36,true,100);edge(sample,36,false,300);
 for(int voice=0;voice<4;++voice)assert(sample.desiredLive(40*voice).value==35);
 assert(!sample.desiredLive(160).known&&!sample.desiredLive(1328).known);flush(sample);
 // The other group is unknown: not even its first parameter may change.
 edge(sample,37,true,310);edge(sample,37,false,510);
 assert(!sample.desiredLive(160).known&&!sample.nextSourcePacket().valid);
 // A numeric default bound to one alternative must never reset a different instrument.
 HostEngine alternate;ResetDefaults altDefaults;altDefaults.config=1;assert(altDefaults.set(128,61));
 assert(alternate.requestResetDefaults(altDefaults));alternate.beginRenderBuffer();
 SourcePatchBank::Values base{};for(unsigned f=0;f<60;++f)base[f]=f%30<18?23:0;
 assert(alternate.configureAlternativeBase(0,0,base,1)&&alternate.bindAlternativeSlot(0,0,0));
 alternate.observeTransport(false);assert(alternate.requestAlternative(0,0)==AlternativeArbiter::Request::Accepted);flush(alternate);
 assert(alternate.activeAlternative(0)==0);
 std::array<ContinuousTarget,97> machine{};machine[0]={320,128};
 assert(!alternate.resetContinuousTargets(machine,1)&&alternate.desiredLive(320).value==23);
 altDefaults.alternatives[0]=0;assert(alternate.requestResetDefaults(altDefaults));alternate.beginRenderBuffer();
 assert(alternate.resetContinuousTargets(machine,1)&&alternate.desiredLive(320).value==61);flush(alternate);
 // Session defaults wait for the same true-UP boundary as the musical restore.
 HostEngine delayed;ResetDefaults before;before.config=1;assert(before.set(708,63));
 assert(delayed.requestResetDefaults(before));delayed.beginRenderBuffer();edge(delayed,55,true,10);
 auto restore=delayed.documentForHost();ResetDefaults after=before;assert(after.set(708,37));
 assert(delayed.requestDocumentRestore(restore)&&delayed.requestResetDefaults(after,true));
 delayed.beginRenderBuffer();
 std::array<ContinuousTarget,97> filter{};filter[0]={1476,708};
 assert(delayed.resetContinuousTargets(filter,1)&&delayed.desiredLive(1476).value==63);flush(delayed);
 edge(delayed,55,false,20);flush(delayed);delayed.beginRenderBuffer();
 assert(delayed.resetContinuousTargets(filter,1)&&delayed.desiredLive(1476).value==37);flush(delayed);
 // Numeric reset is also admissible while a qualified LONG context owns its NAV archive.
 HostEngine moment;ResetDefaults md;md.config=1;assert(md.set(708,63)&&md.set(907,41));
 assert(moment.requestResetDefaults(md));moment.beginRenderBuffer();
 edge(moment,13,true,10);edge(moment,14,true,20);moment.advance(220,10560);
 assert(moment.navMode()==3&&moment.longNavPending());tap(moment,55,230);
 assert(moment.desiredLive(1476).known&&moment.desiredLive(1476).value==63);flush(moment);
 edge(moment,14,false,250);edge(moment,13,false,260);
 assert(!moment.longNavPending()&&moment.navMode()==0&&moment.desiredLive(1476).value==63);
 edge(moment,14,true,300);edge(moment,15,true,310);moment.advance(510,24480);
 assert(moment.navMode()==5&&moment.longNavPending());tap(moment,48,520);
 assert(moment.desiredLive(1744).known&&moment.desiredLive(1744).value==41);flush(moment);
 edge(moment,15,false,540);edge(moment,14,false,550);
 assert(!moment.longNavPending()&&moment.navMode()==0&&moment.desiredLive(1744).value==41);
 std::cout<<"PASS configured RESET FILTER GLOBAL, atomic scopes, UNKNOWN/config refusal, retry, PRESET/STOP and SPOT continuous reset\n";
}
