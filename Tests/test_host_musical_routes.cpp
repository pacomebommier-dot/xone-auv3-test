#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void edge(HostEngine& e,int note,bool down,int ms) {
    const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};
    assert(!e.receive(0,b,3,ms,ms).emit);
}
static Result cc(HostEngine& e,int number,int value,int ms) {
    const uint8_t b[]{0xBE,static_cast<uint8_t>(number),static_cast<uint8_t>(value)};return e.receive(0,b,3,ms,ms);
}
static void loopMode(HostEngine& e) {edge(e,14,true,0);edge(e,13,true,10);edge(e,13,false,20);edge(e,14,false,30);assert(e.navMode()==4);}
static SessionImage image() {SessionImage i{};i.schema=1;i.config=1;i.revision=1;i.desired.fill(-2);return i;}
int main() {
    {
        HostEngine e;auto i=image();i.desired[1744]=64;i.desired[1552]=64;assert(e.restoreSessionBeforeRendering(i));
        auto r=cc(e,4,64,0);assert(r.emit&&r.api==907);e.sent(r,0);
        edge(e,36,true,10);edge(e,36,false,20);
        r=cc(e,4,64,30);assert(r.emit&&r.api==715);e.sent(r,0); // source selection, not bus FX
        edge(e,36,true,100);e.stop(101);edge(e,36,false,110);
        r=cc(e,4,65,120);assert(r.emit&&r.api==715);e.sent(r,0); // STOP did not fabricate short release
        edge(e,36,true,130);edge(e,36,false,140);
        r=cc(e,4,70,150);assert(r.emit&&r.api==907);
    }
    {
        HostEngine e;auto i=image();i.desired[1337]=127;i.desired[1338]=64;i.desired[1339]=32;i.desired[1341]=127;
        assert(e.restoreSessionBeforeRendering(i));loopMode(e);
        auto r=cc(e,19,127,40);assert(r.packetCount()==3);
        const int initial[]{127,64,32};
        for(unsigned k=0;k<3;++k){auto p=r.packet(k);assert(p.api==569+static_cast<int>(k)&&p.bytes[0]==0xBA&&p.bytes[1]==57+k&&p.bytes[2]==initial[k]);e.sent(p,0);}
        r=cc(e,19,64,100);assert(r.packetCount()==3);
        const int gains[]{64,32,16};
        for(unsigned k=0;k<3;++k){auto p=r.packet(k);assert(p.api==569+static_cast<int>(k)&&p.bytes[2]==gains[k]);e.sent(p,k==1?-1:0);}
        r=cc(e,19,64,101);assert(r.packetCount()==1&&r.api==570&&r.value==32);e.sent(r.packet(0),0);
        assert(!cc(e,19,64,102).emit);
        assert(!cc(e,16,127,200).emit); // same derived gain, captures pickup only
        r=cc(e,16,64,201);assert(r.packetCount()==1&&r.api==569&&r.value==32);
        assert(e.desiredLive(1337).value==64 && e.desiredLive(1338).value==64 && e.desiredLive(1341).value==64);
    }
    {
        HostEngine e;auto i=image();i.desired[1337]=127;assert(e.restoreSessionBeforeRendering(i));loopMode(e);
        assert(!cc(e,16,127,40).emit);assert(!cc(e,19,64,50).emit);
        assert(!e.desiredLive(1341).known && e.observed()==-2); // no invented multiplier
    }
    std::cout<<"PASS physical source selection and LOOP same-revision derived gains, batch send failure, UNKNOWN\n";
}
