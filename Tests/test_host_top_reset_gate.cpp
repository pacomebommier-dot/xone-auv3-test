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
    // TOP1..4 are buttons distinct from DJ1/DJ2 rotations. They are
    // source-bound RESET intents, NOT unknown default values.
    for(int n=52;n<=55;++n){
        const int t=10*(n-52);
        assert(!edge(e,n,true,t).emit);
        assert(!edge(e,n,false,t+1).emit);
    }
    assert(e.topResetsDeferred()==4);
    assert(!edge(e,52,false,100).emit);
    assert(e.topResetsDeferred()==4); // orphan UP cannot reset
    edge(e,52,true,110);
    e.stop(111*48);
    edge(e,52,false,120);
    assert(e.topResetsDeferred()==4); // STOP cancels ownership
    assert(!e.nextStopPacket().emit);
    std::cout<<"PASS TOP source-bound reset intent, no guessed MIDI defaults, orphan UP, STOP\n";
}
