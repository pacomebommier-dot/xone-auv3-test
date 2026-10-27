#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
int main(){
 HostEngine e;
 const auto old=e.dj2BackendGenerationForDiagnostic();
 assert(e.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::On,old));
 assert(e.dj2LoopObservedForDiagnostic()==HostEngine::Dj2LoopObserved::On);
 e.stop(48);
 assert(e.dj2BackendGenerationForDiagnostic()!=old);
 assert(e.dj2LoopObservedForDiagnostic()==HostEngine::Dj2LoopObserved::Unknown);
 assert(!e.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::On,old));
 assert(e.dj2LoopObservedForDiagnostic()==HostEngine::Dj2LoopObserved::Unknown);
 const auto now=e.dj2BackendGenerationForDiagnostic();
 assert(e.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::Off,now));
 assert(!e.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::Unknown,now));
 std::cout<<"PASS DJ2 stale backend ACK rejected across STOP generation\n";
}
