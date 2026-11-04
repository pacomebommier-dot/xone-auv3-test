#include "../Shared/XoneSh01aMirror.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
// Synthetic transport identities, deliberately NOT a qualified Roland CC36 list.
static Sh01aProfile fixture(){Sh01aProfile p;p.destination=17;p.channel=0;p.cable=1;p.windowMs=100;for(unsigned i=0;i<36;++i)p.cc[i]=uint8_t(i+1);return p;}
static void pc(Sh01aMirror& m,int program,int64_t t){const uint8_t b[]{0xC0,uint8_t(program)};assert(m.receive(1,b,2,t).accepted);}
static void cc(Sh01aMirror& m,unsigned n,int value,int64_t t){const uint8_t b[]{0xB0,uint8_t(n),uint8_t(value)};assert(m.receive(1,b,3,t).accepted);}
static void full(Sh01aMirror& m,int value,int64_t t){for(unsigned i=36;i>0;--i)cc(m,i,value,t);}
int main(){
 auto p=fixture();assert(p.valid());auto bad=p;bad.cc[35]=bad.cc[0];assert(!bad.valid());
 Sh01aMirror m;assert(!m.configure(bad)&&m.configure(p));
 pc(m,0,0);for(unsigned i=1;i<=35;++i)cc(m,i,0,1);assert(m.received()==35&&!m.snapshot().mirrors[0].valid);
 for(unsigned i=0;i<20;++i)cc(m,1,127,2);
 assert(m.received()==35);
 const uint8_t foreign[]{0xB0,90,33};assert(!m.receive(1,foreign,3,2).accepted);
 const uint8_t right[]{0xB0,36,127};assert(!m.receive(0,right,3,2).accepted);
 cc(m,36,127,3);assert(m.snapshot().mirrors[0].valid&&m.snapshot().mirrors[0].revision==1);
 assert(m.snapshot().mirrors[0].values[0]==127&&m.snapshot().mirrors[0].values[35]==127);
 cc(m,1,22,4);assert(m.live(1)==22&&m.snapshot().mirrors[0].values[0]==127&&m.altered(0));
 pc(m,0,10);cc(m,1,42,11);pc(m,1,12);for(unsigned i=2;i<=36;++i)cc(m,i,17,13);
 assert(!m.snapshot().mirrors[1].valid&&m.received()==35);m.advance(113);assert(!m.capturing()&&m.incomplete(1));
 assert(m.snapshot().mirrors[0].revision==1);
 pc(m,2,120);full(m,127,121);assert(m.snapshot().mirrors[2].valid&&m.snapshot().mirrors[2].values[5]==127);
 auto old=m.snapshot();pc(m,63,130);full(m,44,131);assert(m.snapshot()==old&&m.live(6)==44);
 pc(m,0,140);full(m,19,141);assert(m.snapshot().mirrors[0].revision==2&&!m.altered(0));
 pc(m,0,150);cc(m,1,7,151);m.localEdit(2,88);full(m,50,152);assert(m.incomplete(0)&&m.snapshot().mirrors[0].revision==2);
 pc(m,1,160);cc(m,1,9,161);m.stop();full(m,8,162);assert(!m.snapshot().mirrors[1].valid);
 // Repeated PC can never pool distinct CC across two epochs into a false complete.
 pc(m,1,170);for(unsigned i=1;i<=20;++i)cc(m,i,1,171);pc(m,1,172);for(unsigned i=21;i<=36;++i)cc(m,i,2,173);
 assert(!m.snapshot().mirrors[1].valid&&m.received()==16);
 Sh01aMirror timing;assert(timing.configure(p));pc(timing,0,0);full(timing,5,1);
 pc(timing,1,2);assert(timing.live(1)==-2&&!timing.altered(0));
 timing.advance(200);const uint8_t stale[]{0xC0,0};assert(!timing.receive(1,stale,2,199).accepted);
 auto other=p;other.destination=99;assert(!timing.configure(other));
 auto exhausted=timing.snapshot();exhausted.mirrors[0].revision=UINT32_MAX;
 assert(timing.restore(exhausted));pc(timing,0,300);full(timing,4,301);
 assert(timing.snapshot()==exhausted&&timing.incomplete(0));
 auto saved=m.snapshot();Sh01aMirrorBytes wire{};assert(encodeSh01aMirror(saved,wire));Sh01aMirrorImage image;
 assert(decodeSh01aMirror(wire.data(),wire.size(),image)&&image==saved);
 Sh01aMirror reopen;assert(reopen.restore(image)&&!reopen.capturing()&&reopen.program()==-1&&reopen.live(1)==-2);
 assert(reopen.snapshot()==saved);wire[40]^=1;assert(!decodeSh01aMirror(wire.data(),wire.size(),image));
 assert(image==saved); // reject without corrupting the accepted destination image
 std::cout<<"PASS SH36 synthetic protocol: distinct staging, atomic mirrors, interrupted epochs, no WRITE proof, CRC persistence; not Roland qualification or AU integration\n";
}
