#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
#include <utility>
using namespace xone::host;
static Result turn(HostEngine& e,int cc,int raw,int at){
    const uint8_t b[]{0xBE,static_cast<uint8_t>(cc),static_cast<uint8_t>(raw)};
    return e.receive(0,b,3,at,at);
}
static int takeDelta(HostEngine& e,int cc){
    Trace t{};int result=0,seen=0;
    while(e.pop(t))if(t.reason==Reason::DjRelative){
        assert(t.stage==Stage::Gesture&&t.bytes[1]==cc);
        result=t.result;++seen;
    }
    assert(seen==1);return result;
}
int main(){
    for(int cc:{20,21}){
        HostEngine e;
        for(auto p:{std::pair<int,int>{1,1},{2,2},{63,63},{64,-64},{65,-63},{126,-2},{127,-1}}){
            assert(!turn(e,cc,p.first,p.first).emit);
            assert(takeDelta(e,cc)==p.second);
        }
        assert(!turn(e,cc,0,128).emit);
        Trace t{};while(e.pop(t))assert(t.reason!=Reason::DjRelative);
        assert(!e.nextStopPacket().emit);
    }
    HostEngine e;e.enableNativeMutePage(true);
    assert(!turn(e,20,1,1).emit);assert(takeDelta(e,20)==1);
    assert(!turn(e,21,127,2).emit);assert(takeDelta(e,21)==-1);
    assert(!e.muteGesturePending()&&!e.mutePageVisible());
    std::cout<<"PASS relative K2 DJ signed input, no pickup, no fabricated output or BOT\n";
}
