#include "../Shared/XoneSh01aProfile.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
// Synthetic transport identities, deliberately NOT a qualified Roland CC36 list.
static Sh01aProfile fixture(bool provisional){Sh01aProfile p;p.destination=17;p.channel=0;p.cable=1;p.windowMs=100;for(unsigned i=0;i<36;++i)p.cc[i]=uint8_t(i+1);if(provisional)p.cc=Sh01aProvisionalCc36;return p;}
static void pc(Sh01aMirror& m,int program,int64_t t){const uint8_t b[]{0xC0,uint8_t(program)};assert(m.receive(1,b,2,t).accepted);}
static void cc(Sh01aMirror& m,unsigned n,int value,int64_t t){const uint8_t b[]{0xB0,m.snapshot().profile.cc[n-1],uint8_t(value)};assert(m.receive(1,b,3,t).accepted);}
static void full(Sh01aMirror& m,int value,int64_t t){for(unsigned i=36;i>0;--i)cc(m,i,value,t);}
static int live(const Sh01aMirror& m,unsigned n){return m.live(m.snapshot().profile.cc[n-1]);}
int main(){
 for(bool provisional:{false,true}){
 auto p=fixture(provisional);assert(p.valid());auto bad=p;bad.cc[35]=bad.cc[0];assert(!bad.valid());
 Sh01aMirror m;assert(!m.configure(bad)&&m.configure(p));
 pc(m,1,0);for(unsigned i=1;i<=35;++i)cc(m,i,0,1);assert(m.received()==35&&!m.snapshot().mirrors[0].valid);
 for(unsigned i=0;i<20;++i)cc(m,1,127,2);
 assert(m.received()==35);
 const uint8_t foreign[]{0xB0,90,33};assert(!m.receive(1,foreign,3,2).accepted);
 const uint8_t right[]{0xB0,36,127};assert(!m.receive(0,right,3,2).accepted);
 cc(m,36,127,3);assert(m.snapshot().mirrors[0].valid&&m.snapshot().mirrors[0].revision==1);
 assert(m.snapshot().mirrors[0].values[0]==127&&m.snapshot().mirrors[0].values[35]==127);
 cc(m,1,22,4);assert(live(m,1)==22&&m.snapshot().mirrors[0].values[0]==127&&m.altered(0));
 pc(m,1,10);cc(m,1,42,11);pc(m,2,12);for(unsigned i=2;i<=36;++i)cc(m,i,17,13);
 assert(!m.snapshot().mirrors[1].valid&&m.received()==35);m.advance(113);assert(!m.capturing()&&m.incomplete(1));
 const auto incomplete=m.takeIncomplete();
 assert(incomplete.program==2&&incomplete.count==35);
 assert(incomplete.missing[p.cc[0]/64]==(uint64_t{1}<<(p.cc[0]%64)));
 assert(!incomplete.extra[0]&&!incomplete.extra[1]&&m.takeIncomplete().program==-1);
 // Extra CC are unique, route-qualified and scoped to the interrupted recall.
 pc(m,2,114);cc(m,1,17,115);
 assert(!m.receive(0,foreign,3,115).accepted);
 assert(!m.receive(1,foreign,3,115).accepted);
 assert(!m.receive(1,foreign,3,115).accepted);
 pc(m,3,116);
 const auto interrupted=m.takeIncomplete();
 assert(interrupted.program==2&&interrupted.count==1);
 assert(interrupted.extra[0]==0&&interrupted.extra[1]==(uint64_t{1}<<(90-64)));
 assert(!(interrupted.missing[p.cc[0]/64]&(uint64_t{1}<<(p.cc[0]%64))));
 assert(m.snapshot().mirrors[0].revision==1);
 pc(m,3,120);full(m,127,121);assert(m.snapshot().mirrors[2].valid&&m.snapshot().mirrors[2].values[5]==127);
 auto old=m.snapshot();pc(m,0,129);full(m,55,129);assert(m.snapshot()==old);pc(m,63,130);full(m,44,131);assert(m.snapshot()==old&&live(m,6)==44);
 pc(m,1,140);full(m,19,141);assert(m.snapshot().mirrors[0].revision==2&&!m.altered(0));
 pc(m,1,150);cc(m,1,7,151);m.localEdit(p.cc[1],88);full(m,50,152);assert(m.incomplete(0)&&m.snapshot().mirrors[0].revision==2);
 pc(m,2,160);cc(m,1,9,161);m.stop();full(m,8,162);assert(!m.snapshot().mirrors[1].valid);
 // Repeated PC can never pool distinct CC across two epochs into a false complete.
 pc(m,2,170);for(unsigned i=1;i<=20;++i)cc(m,i,1,171);pc(m,2,172);for(unsigned i=21;i<=36;++i)cc(m,i,2,173);
 assert(!m.snapshot().mirrors[1].valid&&m.received()==16);
 Sh01aMirror timing;assert(timing.configure(p));pc(timing,1,0);full(timing,5,1);
 pc(timing,2,2);assert(live(timing,1)==-2&&!timing.altered(0));
 timing.advance(200);const uint8_t stale[]{0xC0,0};assert(!timing.receive(1,stale,2,199).accepted);
 auto other=p;other.destination=99;assert(!timing.configure(other));
 auto exhausted=timing.snapshot();exhausted.mirrors[0].revision=UINT32_MAX;
 assert(timing.restore(exhausted));pc(timing,1,300);full(timing,4,301);
 assert(timing.snapshot()==exhausted&&timing.incomplete(0));
 auto saved=m.snapshot();Sh01aMirrorBytes wire{};assert(encodeSh01aMirror(saved,wire));Sh01aMirrorImage image;
 assert(decodeSh01aMirror(wire.data(),wire.size(),image)&&image==saved);
 Sh01aMirror reopen;assert(reopen.restore(image)&&!reopen.capturing()&&reopen.program()==-1&&live(reopen,1)==-2);
 assert(reopen.snapshot()==saved);wire[40]^=1;assert(!decodeSh01aMirror(wire.data(),wire.size(),image));
 assert(image==saved); // reject without corrupting the accepted destination image
 }
 std::cout<<"PASS SH36 synthetic and provisional Roland profiles: distinct staging, atomic mirrors, interrupted epochs, no WRITE proof, CRC persistence; not Roland qualification or AU integration\n";
}
