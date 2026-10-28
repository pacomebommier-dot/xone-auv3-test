#include "../Shared/XoneTopPulseBus.hpp"
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void press(HostEngine& h,int physical,int ms){
    const unsigned note=static_cast<unsigned>(20+physical);
    const std::uint8_t down[]{0x9E,static_cast<std::uint8_t>(note),127};
    const std::uint8_t up[]{0x8E,static_cast<std::uint8_t>(note),0};
    assert(!h.receive(0,down,3,ms*48,ms).emit);
    assert(!h.receive(0,up,3,(ms+1)*48,ms+1).emit);
}
static void verify(TopPulsePacket p,unsigned api,bool on){
    assert(p.valid&&p.api==api&&p.on==on);
    assert(p.bytes[0]==(on?0x90:0x80)+6+api/128);
    assert(p.bytes[1]==api%128&&p.bytes[2]==(on?127:0));
}
int main(){
    TopPulseBus b;
    const unsigned limits[]{4,8,2,8};
    const unsigned starts[]{680,684,692,694};
    for(unsigned f=0;f<4;++f){
        assert(!b.queue(static_cast<TopPulseFamily>(f),limits[f]));
        for(unsigned n=0;n<limits[f];++n)
            assert(b.queue(static_cast<TopPulseFamily>(f),n));
    }
    assert(b.pending()==22);
    assert(!b.queue(static_cast<TopPulseFamily>(4),0));
    for(unsigned f=0;f<4;++f)
        for(unsigned n=0;n<limits[f];++n){
            const auto on=b.next();verify(on,starts[f]+n,true);
            assert(!b.acknowledge({true,true,on.serial+1,on.api,{}},true)); // stale token
            assert(b.acknowledge(on,true));
            auto off=b.next();verify(off,starts[f]+n,false);
            assert(b.acknowledge(off,false)); // failed OFF stays due
            verify(b.next(),starts[f]+n,false);
            assert(b.acknowledge(off,true));
        }
    assert(b.pending()==0 && !b.next().valid);
    // One failed ON may have sounded: close with OFF, never retry the ON.
    assert(b.queue(TopPulseFamily::Init,2));
    auto p=b.next();verify(p,682,true);assert(b.acknowledge(p,false));
    verify(b.next(),682,false);assert(b.hasOwedOff());
    b.stop();verify(b.next(),682,false);
    assert(b.acknowledge(b.next(),true)&&!b.hasOwedOff());
    // No old non-attempted TOP may become a ghost ON after STOP.
    assert(b.queue(TopPulseFamily::Init,0));
    assert(b.queue(TopPulseFamily::Fill,7));
    b.stop();assert(b.pending()==0);
    // Exact physical S3 default INIT, no FX/preset/plugin binding.
    HostEngine e;
    press(e,32,10);press(e,33,20);press(e,34,30);
    assert(e.pendingTopPulses()==3);
    HostEngine::TopIntent def{};
    assert(!e.lastTopIntent(0,def)); // not an invented RESET_DEFAULT
    verify(e.nextTopPulse(),680,true);
    p=e.nextTopPulse();assert(p.valid);e.ackTopPulse(p,true,31*48);
    p=e.nextTopPulse();verify(p,680,false);e.ackTopPulse(p,true,31*48);
    p=e.nextTopPulse();verify(p,681,true);e.ackTopPulse(p,true,31*48);
    p=e.nextTopPulse();verify(p,681,false);e.ackTopPulse(p,true,31*48);
    p=e.nextTopPulse();verify(p,682,true);e.ackTopPulse(p,true,31*48);
    p=e.nextTopPulse();verify(p,682,false);e.ackTopPulse(p,true,31*48);
    assert(!e.nextTopPulse().valid);
    // TOP4 keeps its FILTER GLOBAL intent: missing RESET_DEFAULT never -> 0.
    press(e,35,40);assert(e.topResetsDeferred()==1);
    assert(e.lastTopIntent(3,def)&&def.action==HostEngine::TopAction::FilterReset);
    assert(!e.desiredLive(1476).known);
    // STOP retains OFF after a possibly delivered ON and cancels queued ONs.
    press(e,32,50);p=e.nextTopPulse();verify(p,680,true);
    e.ackTopPulse(p,false,51*48);e.stop(52*48);
    p=e.nextTopPulse();verify(p,680,false);
    e.ackTopPulse(p,false,52*48);
    verify(e.nextTopPulse(),680,false);
    e.ackTopPulse(e.nextTopPulse(),true,53*48);
    assert(!e.nextTopPulse().valid);
    std::cout<<"PASS S7 TOP 22-address pulse bus, physical INIT, FIFO, callback failure and STOP OFF debt\n";
}
