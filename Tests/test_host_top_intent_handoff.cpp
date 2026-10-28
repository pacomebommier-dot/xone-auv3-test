#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void edge(HostEngine& e,int note,bool down,int ms){
 const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),
    static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};
 assert(!e.receive(0,b,3,ms*48,ms).emit);
}
int main(){
 HostEngine e;HostEngine::TopIntent a{},b{};
 // Only TOP4 remains a deferred FILTER GLOBAL intent on normal INIT.
 // TOP1..3 now produce immediate neutral MIDI action pulses.
 assert(!e.consumeTopIntent(3,1,b));
 edge(e,55,true,10);edge(e,55,false,20);
 assert(e.lastTopIntent(3,a));
 assert(!e.consumeTopIntent(3,a.sequence+1,b));
 assert(e.lastTopIntent(3,b));
 assert(e.consumeTopIntent(3,a.sequence,b));
 assert(b.sequence==a.sequence&&b.physical==35);
 assert(!e.consumeTopIntent(3,a.sequence,b));
 edge(e,55,true,30);edge(e,55,false,31);
 assert(e.lastTopIntent(3,a));
 edge(e,55,true,40);edge(e,55,false,41);
 assert(!e.consumeTopIntent(3,a.sequence,b));
 assert(e.lastTopIntent(3,a));
 assert(e.consumeTopIntent(3,a.sequence,b));
 edge(e,55,true,50);edge(e,55,false,51);
 assert(e.lastTopIntent(3,a));
 e.stop(52*48);
 assert(!e.consumeTopIntent(3,a.sequence,b));
 assert(!e.nextStopPacket().emit);
 std::cout<<"PASS TOP one-shot deferred handoff, stale epoch rejection and STOP cancellation\n";
}
