#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void note(HostEngine& e,int n,bool down,int ms){
 const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(n),static_cast<uint8_t>(down?127:0)};
 e.receive(0,b,3,ms*48,ms);
}
static void push(HostEngine& e,bool down,int ms){note(e,14,down,ms);}
static void turn(HostEngine& e,int ms){
 const uint8_t b[]{0xBE,21,1};auto r=e.receive(0,b,3,ms*48,ms);assert(r.emit);const auto packet=r.packet(0);assert(packet.dj2Qualified==r.dj2Qualified);assert(packet.gestureMs==r.gestureMs);e.sent(packet,0);
}
int main(){
 HostEngine e;
 push(e,true,10);turn(e,20);push(e,false,30);assert(e.navMode()==2);
 assert(e.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::On));
 push(e,true,40);push(e,false,50);
 assert(e.navMode()==2); // ON PUSH must stop DJ LOOP while keeping GLOBAL.
 assert(e.dj2LoopStopRequestsForDiagnostic()==1);
 assert(e.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::Off));
 push(e,true,60);push(e,false,70);assert(e.navMode()==0);
 // GLOBAL rotations retain GLOBAL; a service handoff is never an audio ACK.
 HostEngine rotating;
 push(rotating,true,10);turn(rotating,20);push(rotating,false,30);
 auto start=rotating.pendingDjLoopService();
 assert(start.command==HostEngine::DjLoopCommand::Start);
 assert(rotating.acknowledgeDjLoopService(start));
 assert(!rotating.acknowledgeDjLoopService(start));
 assert(rotating.dj2LoopObservedForDiagnostic()==HostEngine::Dj2LoopObserved::Unknown);
 assert(rotating.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::On));
 push(rotating,true,40);turn(rotating,50);push(rotating,false,60);
 assert(rotating.navMode()==2 && rotating.dj2LoopStopRequestsForDiagnostic()==0);
 // GLOBAL -> LOOP is a combo: no premature solo STOP and no reset.
 note(rotating,14,true,70);note(rotating,13,true,80);
 note(rotating,13,false,90);note(rotating,14,false,100);
 assert(rotating.navMode()==4 && rotating.dj2LoopStopRequestsForDiagnostic()==0);
 push(rotating,true,110);turn(rotating,120);push(rotating,false,130);
 assert(rotating.navMode()==4); // qualified LOOP gesture retains LOOP
 push(rotating,true,140);push(rotating,false,150);
 assert(rotating.navMode()==4 && rotating.dj2LoopStopRequestsForDiagnostic()==1);
 // A genuine navigation out of LOOP requests STOP/RESET, not a LOOP1/2/3 action.
 const auto beforeExit=rotating.dj2BackendGenerationForDiagnostic();
 note(rotating,15,true,160);note(rotating,15,false,170);
 assert(rotating.navMode()==0);
 auto reset=rotating.pendingDjLoopService();
 assert(reset.command==HostEngine::DjLoopCommand::StopReset);
 assert(!rotating.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::On,beforeExit));
 assert(!rotating.acknowledgeDjLoopService(start));
 // UNKNOWN cannot be treated as verified OFF for solo page exit.
 HostEngine unknown;push(unknown,true,10);push(unknown,false,20);assert(unknown.navMode()==2);
 push(unknown,true,30);push(unknown,false,40);assert(unknown.navMode()==2);
 assert(unknown.dj2LoopStopRequestsForDiagnostic()==0);
 // Strict 200ms threshold: late rotation never requests a performance loop.
 HostEngine late;push(late,true,10);turn(late,210);push(late,false,220);
 assert(!late.dj2LoopStartRequestedForDiagnostic());
 // Holding solo PUSH must stop once, without a temporary GLOBAL exit.
 push(rotating,true,180);push(rotating,false,190);assert(rotating.navMode()==2);
 const auto stopCount=rotating.dj2LoopStopRequestsForDiagnostic();
 push(rotating,true,200);rotating.advance(400,400*48);
 assert(rotating.navMode()==2 && !rotating.longNavPending());
 assert(rotating.dj2LoopStopRequestsForDiagnostic()==stopCount+1);
 push(rotating,false,410);assert(rotating.navMode()==2);
 assert(rotating.dj2LoopStopRequestsForDiagnostic()==stopCount+1);
 // The native MUTE overlay is a departure even when NAV mode is unchanged.
 HostEngine mute;mute.enableNativeMutePage(true);
 push(mute,true,10);turn(mute,20);push(mute,false,30);
 assert(mute.observeDj2LoopBackendForDiagnostic(HostEngine::Dj2LoopObserved::On));
 note(mute,13,true,40);note(mute,13,false,50);
 assert(mute.mutePageVisible());
 assert(mute.pendingDjLoopService().command==HostEngine::DjLoopCommand::StopReset);
 std::cout<<"PASS DJ2 GLOBAL ON PUSH stops without exiting, OFF PUSH exits\n";
}
