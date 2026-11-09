#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void bot(HostEngine& e,bool down,int ms){
 const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),14,
  static_cast<uint8_t>(down?127:0)};
 e.receive(0,b,3,ms*48,ms);
}
static void tick(HostEngine& e,int value,int ms){
 const uint8_t b[]{0xBE,21,static_cast<uint8_t>(value)};
 auto r=e.receive(0,b,3,ms*48,ms);assert(r.emit);e.sent(r,0);
}
int main(){
 HostEngine e;
 assert(e.dj2LoopObservedForDiagnostic()==HostEngine::Dj2LoopObserved::Unknown);
 assert(!e.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::Unknown));
 bot(e,true,10);tick(e,1,20);
 assert(e.dj2LoopRequestsForDiagnostic()==1);
 assert(e.dj2LoopObservedForDiagnostic()==HostEngine::Dj2LoopObserved::Unknown);
 assert(e.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::On));
 tick(e,2,30);assert(e.dj2LoopRequestsForDiagnostic()==1);
 assert(e.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::Off));
 tick(e,1,40);assert(e.dj2LoopRequestsForDiagnostic()==2);
 bot(e,false,50);e.stop(51*48);
 assert(!e.dj2LoopStartRequestedForDiagnostic());
 std::cout<<"PASS DJ2 backend observed state separate from source activation requests\n";
}
