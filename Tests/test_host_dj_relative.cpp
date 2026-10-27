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
    // S7 virtual API kind16 is CC51/52 on MIDI channel14 (0xBD).
    for(int cc:{20,21}){
        HostEngine e;int now=0;
        for(auto p:{std::pair<int,int>{1,1},{1,1},{2,2},{63,63},
                    {64,-64},{65,-63},{126,-2},{127,-1}}){
            auto r=turn(e,cc,p.first,++now);
            assert(r.emit&&r.musical&&r.type==16&&r.api==947+cc-20);
            assert(r.bytes[0]==0xBD&&r.bytes[1]==51+cc-20&&r.bytes[2]==p.first);
            assert(r.packetCount()==1&&e.canSend(r));
            assert(takeDelta(e,cc)==p.second);
            e.sent(r,0);
        }
        assert(!turn(e,cc,0,128).emit);
        Trace t{};while(e.pop(t))assert(t.reason!=Reason::DjRelative);
        assert(!e.nextStopPacket().emit);
    }
    // Failed callback must not poison a future identical delta or create debt.
    {
        HostEngine e;
        auto a=turn(e,20,1,10);assert(a.emit);
        e.sent(a,-1);
        auto b=turn(e,20,1,11);assert(b.emit&&b.bytes[2]==1);
        e.sent(b,0);
        assert(!e.nextStopPacket().emit);
    }
    // DJ rotation never forges BOT2/MUTE or BOT3/GLOBAL push.
    {
        HostEngine e;e.enableNativeMutePage(true);
        auto a=turn(e,20,1,1);assert(a.emit);e.sent(a,0);
        auto b=turn(e,21,127,2);assert(b.emit);e.sent(b,0);
        assert(!e.muteGesturePending()&&!e.mutePageVisible());
    }
    std::cout<<"PASS S7 DJ1/DJ2 real MIDI output, relative signed deltas, no dedup/pickup, callback failure\n";
}
