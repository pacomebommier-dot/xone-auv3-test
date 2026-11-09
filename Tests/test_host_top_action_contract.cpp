#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void press(HostEngine& e,int note,int ms){
 const uint8_t d[]{0x9E,static_cast<uint8_t>(note),127};
 const uint8_t u[]{0x8E,static_cast<uint8_t>(note),0};
 assert(!e.receive(0,d,3,ms*48,ms).emit);
 assert(!e.receive(0,u,3,(ms+1)*48,ms+1).emit);
}
int main(){
 HostEngine e;
 // Source GOLDEN: bare INIT TOP1..3 are transient virtual pulses.
 // TOP4 is the separate invariant FILTER GLOBAL request, still UNKNOWN safe.
 for(int n=52;n<56;++n)press(e,n,10*(n-52));
 assert(e.pendingTopPulses()==3);
 assert(e.topResetsDeferred()==1&&e.topPatchRecallsDeferred()==0);
 for(unsigned i=0;i<3;++i){HostEngine::TopIntent a{};assert(!e.lastTopIntent(i,a));}
 HostEngine::TopIntent top4{};assert(e.lastTopIntent(3,top4));
 assert(top4.action==HostEngine::TopAction::FilterReset);
 // Context classification is explicit and does not transform an old
 // unconsumed RESET into an arbitrary PATCH callback.
 // Dedicated SAMPLE/MACHINE navigation scenarios remain in S3 tests.
 e.stop(100*48);
 for(unsigned i=0;i<4;++i){HostEngine::TopIntent a{};assert(!e.lastTopIntent(i,a));}
 std::cout<<"PASS TOP action class and captured destination with UNKNOWN-safe dispatch\n";
}
