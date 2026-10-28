#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static Result edge(HostEngine& e,int n,bool down,int ms){
    const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),
       static_cast<uint8_t>(n),static_cast<uint8_t>(down?127:0)};
    return e.receive(0,b,3,ms*48,ms);
}
int main(){
    HostEngine e;
    // TOP1..3 are INIT action pulses. Invariant TOP4 is a source-bound
    // FILTER GLOBAL request; UNKNOWN default cannot manufacture a CC.
    for(int n=52;n<=55;++n){
        const int t=10*(n-52);
        assert(!edge(e,n,true,t).emit);
        assert(!edge(e,n,false,t+1).emit);
    }
    assert(e.topResetsDeferred()==1 && e.pendingTopPulses()==3);
    for(unsigned i=0;i<3;++i){HostEngine::TopIntent intent{};assert(!e.lastTopIntent(i,intent));}
    HostEngine::TopIntent filter{};
    assert(e.lastTopIntent(3,filter));
    assert(filter.physical==35&&filter.route.address>=0&&filter.sequence>0);
    assert(!edge(e,52,false,100).emit);
    assert(e.topResetsDeferred()==1); // orphan UP cannot reset
    edge(e,52,true,110);
    e.stop(111*48);
    edge(e,52,false,120);
    assert(e.topResetsDeferred()==1); // STOP cancels ownership
    for(unsigned i=0;i<4;++i){
        HostEngine::TopIntent intent{};
        assert(!e.lastTopIntent(i,intent));
    }
    assert(!e.nextStopPacket().emit);
    std::cout<<"PASS TOP source-bound reset intent, no guessed MIDI defaults, orphan UP, STOP\n";
}
