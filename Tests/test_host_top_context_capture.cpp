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
 HostEngine e;
 // The physical DOWN binds the context as well as the route; the
 // source's route mapping alone may alias across navigation contexts.
 edge(e,52,true,10);edge(e,52,false,20);
 HostEngine::TopIntent a{};
 assert(e.pendingTopPulses()==1&&!e.lastTopIntent(0,a)); // INIT is ACTION, not RESET
 edge(e,53,true,30);e.stop(31*48);edge(e,53,false,40);
 assert(e.pendingTopPulses()==0);
 assert(!e.lastTopIntent(0,a));assert(!e.lastTopIntent(1,a));
 // Source TOP4 never produces a PATCH recall on the normal socle.
 edge(e,55,true,50);edge(e,55,false,51);
 assert(e.lastTopIntent(3,a)&&a.action==HostEngine::TopAction::FilterReset);
 std::cout<<"PASS TOP DOWN-bound route and context, STOP, conservative UNKNOWN-safe intent\n";
}
