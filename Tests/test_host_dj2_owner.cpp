#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static Result input(HostEngine& e,int n,bool down,int ms){
    const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),
        static_cast<uint8_t>(n),static_cast<uint8_t>(down?127:0)};
    return e.receive(0,b,3,ms*48,ms);
}
static Result turn(HostEngine& e,int raw,int ms){
    const uint8_t b[]{0xBE,21,static_cast<uint8_t>(raw)};
    return e.receive(0,b,3,ms*48,ms);
}
int main(){
    // Bare DJ2 relative ticks are sent but cannot forge a push owner.
    {
        HostEngine e;auto r=turn(e,1,10);assert(r.emit);e.sent(r,0);
        assert(e.dj2QualifiedTurnsForDiagnostic()==0&&!e.dj2PushRotationCapturedForDiagnostic());
    }
    // BOT3+DJ2 within 200ms qualifies owner40, preserves signed delta.
    {
        HostEngine e;input(e,14,true,100);
        auto r=turn(e,127,299);assert(r.emit&&r.api==948);
        assert(e.dj2PushRotationCapturedForDiagnostic());
        e.sent(r,0);assert(e.dj2QualifiedTurnsForDiagnostic()==1);
        assert(e.dj2LastQualifiedDeltaForDiagnostic()==-1);
        input(e,14,false,320);
        assert(!e.dj2PushRotationCapturedForDiagnostic());
        r=turn(e,1,330);assert(r.emit);e.sent(r,0);
        assert(e.dj2QualifiedTurnsForDiagnostic()==1);
    }
    // Failed callbacks do not manufacture a backend-confirmed movement.
    {
        HostEngine e;input(e,14,true,100);
        auto r=turn(e,1,110);assert(r.emit);e.sent(r,-1);
        assert(e.dj2QualifiedTurnsForDiagnostic()==0);
        e.stop(111*48);
        assert(!e.dj2PushRotationCapturedForDiagnostic());
        input(e,14,false,120);
        assert(e.dj2QualifiedTurnsForDiagnostic()==0);
    }
    // The callback cannot turn a nonqualified DJ2 packet into a qualified
    // action merely because a newer physical gesture owns BOT3.
    {
        HostEngine e;
        auto unqualified=turn(e,1,10);assert(unqualified.emit&&!unqualified.dj2Qualified);
        input(e,14,true,20);
        auto qualified=turn(e,2,30);assert(qualified.emit&&qualified.dj2Qualified);
        e.sent(unqualified,0);
        assert(e.dj2QualifiedTurnsForDiagnostic()==0);
        e.sent(qualified,0);
        assert(e.dj2QualifiedTurnsForDiagnostic()==1);
    }
    // At 200ms no new push+rot gesture may be qualified.
    {
        HostEngine e;input(e,14,true,100);
        auto r=turn(e,1,300);assert(r.emit);e.sent(r,0);
        assert(e.dj2QualifiedTurnsForDiagnostic()==0);
    }
    std::cout<<"PASS DJ2 owner40 qualification, signed delta, callback failure, expiry and STOP\n";
}
