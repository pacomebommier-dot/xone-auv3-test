#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void push(HostEngine& e,int ms){const uint8_t b[]{0x9E,14,127};e.receive(0,b,3,ms*48,ms);}
static void turn(HostEngine& e,int ms){const uint8_t b[]{0xBE,21,1};auto r=e.receive(0,b,3,ms*48,ms);assert(r.emit);e.sent(r,0);}
int main(){
 HostEngine e;e.stop(1);assert(e.dj2LoopStopRequestsForDiagnostic()==0);
 push(e,10);turn(e,20);
 assert(e.dj2LoopStartRequestedForDiagnostic());
 e.stop(21*48);
 assert(e.dj2LoopStopRequestsForDiagnostic()==1);
 assert(!e.dj2LoopStartRequestedForDiagnostic());
 assert(e.dj2LoopObservedForDiagnostic()==HostEngine::Dj2LoopObserved::Unknown);
 e.stop(22*48);assert(e.dj2LoopStopRequestsForDiagnostic()==1);
 HostEngine f;
 assert(f.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::On));
 f.stop(30*48);assert(f.dj2LoopStopRequestsForDiagnostic()==1);
 assert(f.dj2LoopObservedForDiagnostic()==HostEngine::Dj2LoopObserved::Unknown);
 std::cout<<"PASS STOP requests DJ LOOP termination without fake audio acknowledgement\n";
}
