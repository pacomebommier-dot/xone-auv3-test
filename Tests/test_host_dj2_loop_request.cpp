#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void push(HostEngine& e,bool down,int ms){
 const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),14,
    static_cast<uint8_t>(down?127:0)};
 assert(!e.receive(0,b,3,ms*48,ms).emit);
}
static Result rot(HostEngine& e,int value,int ms){
 const uint8_t b[]{0xBE,21,static_cast<uint8_t>(value)};
 return e.receive(0,b,3,ms*48,ms);
}
int main(){
 HostEngine e;
 auto bare=rot(e,1,10);assert(bare.emit);e.sent(bare,0);
 assert(!e.dj2LoopStartRequestedForDiagnostic());
 push(e,true,20);
 auto first=rot(e,1,30);assert(first.emit&&first.dj2Qualified);e.sent(first,0);
 assert(e.dj2LoopStartRequestedForDiagnostic()&&e.dj2LoopRequestsForDiagnostic()==1);
 auto second=rot(e,127,40);assert(second.emit&&second.dj2Qualified);e.sent(second,0);
 assert(e.dj2LoopRequestsForDiagnostic()==1); // no duplicate start intent
 push(e,false,50);
 auto after=rot(e,1,60);assert(after.emit);e.sent(after,0);
 assert(e.dj2LoopRequestsForDiagnostic()==1);
 e.stop(61*48);
 assert(!e.dj2LoopStartRequestedForDiagnostic());
 assert(e.dj2LoopRequestsForDiagnostic()==1); // historical diagnostic only
 // No output service 19 is manufactured; only the sourced CC52 goes out.
 std::cout<<"PASS Q6 DJ2 qualified request-only activation, no duplicate starts, STOP cancels\n";
}
