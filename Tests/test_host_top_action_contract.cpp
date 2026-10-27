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
 // Default SOCLE TOP1..4: FILTER reset intents, never unknown zero.
 for(int n=52;n<56;++n)press(e,n,10*(n-52));
 assert(e.topResetsDeferred()==4&&e.topPatchRecallsDeferred()==0);
 for(unsigned i=0;i<4;++i){
   HostEngine::TopIntent a{};assert(e.lastTopIntent(i,a));
   assert(a.action==HostEngine::TopAction::FilterReset);
 }
 // Context classification is explicit and does not transform an old
 // unconsumed RESET into an arbitrary PATCH callback.
 // Dedicated SAMPLE/MACHINE navigation scenarios remain in S3 tests.
 e.stop(100*48);
 for(unsigned i=0;i<4;++i){HostEngine::TopIntent a{};assert(!e.lastTopIntent(i,a));}
 std::cout<<"PASS TOP action class and captured destination with UNKNOWN-safe dispatch\n";
}
