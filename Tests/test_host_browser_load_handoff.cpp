#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static uint32_t select(HostEngine& e,int ms){const uint8_t b[]{0xBE,20,1};auto r=e.receive(0,b,3,ms*48,ms);assert(r.emit);e.sent(r.packet(0),0);return r.sequence;}
static void edge(HostEngine& e,bool down,int ms){const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),13,static_cast<uint8_t>(down?127:0)};assert(!e.receive(0,b,3,ms*48,ms).emit);}
int main(){
 HostEngine e;assert(e.configureDjBrowserDeckForDiagnostic(2));e.enableNativeMutePage(true);
 auto selection=select(e,5000);edge(e,true,7999);edge(e,false,8001);
 HostEngine::BrowserLoadRequest load{},taken{};assert(e.pendingBrowserLoad(load));
 assert(load.deck==2&&load.selectionSequence==selection&&load.sequence>selection);
 assert(!e.mutePageVisible());
 assert(!e.consumeBrowserLoad(load.sequence+1,taken));
 assert(e.consumeBrowserLoad(load.sequence,taken)&&taken.deck==2);
 assert(!e.consumeBrowserLoad(load.sequence,taken));
 select(e,9000);edge(e,true,9010);edge(e,false,9020);assert(e.pendingBrowserLoad(load));
 select(e,9030);assert(!e.pendingBrowserLoad(taken)); // no LOAD of a newer selection by an old request
 assert(!e.consumeBrowserLoad(load.sequence,taken));
 edge(e,true,9040);edge(e,false,9050);assert(e.pendingBrowserLoad(load));
 e.stop(9060*48);assert(!e.pendingBrowserLoad(taken));
 assert(!e.consumeBrowserLoad(load.sequence,taken));
 std::cout<<"PASS Browser LOAD one-shot deck/selection handoff, expiry, replacement, STOP and MUTE exclusion\n";
}
