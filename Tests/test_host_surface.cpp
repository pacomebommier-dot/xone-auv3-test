#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
int main() {
    const int notes[]{12,13,14,15,36,37,38,39,32,33,34,35,28,29,30,31,24,25,26,27,48,49,50,51,44,45,46,47,40,41,42,43,52,53,54,55};
    xone::host::HostEngine e;
    int ms=0;
    for(int i=0;i<36;++i) {
        assert(xone::l01::SurfaceInput::physicalControl(notes[i])==i);
        uint8_t d[]{0x9E,static_cast<uint8_t>(notes[i]),127};
        uint8_t u[]{0x8E,static_cast<uint8_t>(notes[i]),0};
        assert(!e.receive(0,d,3,0,ms++).emit && e.surfaceHeld(i));
        e.receive(0,d,3,0,ms++); // duplicate DOWN suppressed
        e.receive(0,u,3,0,ms++);assert(!e.surfaceHeld(i));
    }
    HostEngine ccEngine; // use the default S3 NAV for absolute controls
    for(int cc=0;cc<128;++cc) {
        uint8_t d[]{0xBE,static_cast<uint8_t>(cc),64};
        const auto result=ccEngine.receive(0,d,3,0,ms++);
        // The first 20 absolute CCs are admissible without LIVE fixture.
        if(cc<20){
            assert(result.emit&&result.musical&&result.type==10);
            assert(result.bytes[2]==64);
            ccEngine.sent(result,0);
        }
        // DJ1/DJ2 (CC20/21) retain their independent S7 type16 route.
        // Their relative value 64 is -64, not an absolute CC or a Thru.
        else if(cc==20||cc==21){
            assert(result.emit&&result.musical&&result.type==16);
            assert(result.api==947+cc-20);
            assert(result.bytes[0]==0xBD&&result.bytes[1]==51+cc-20&&result.bytes[2]==64);
            ccEngine.sent(result,0);
        }else assert(!result.emit);
    }
    for(int n=0;n<128;++n) {
        if(xone::l01::SurfaceInput::physicalControl(n)>=0)continue;
        uint8_t d[]{0x9E,static_cast<uint8_t>(n),127};
        e.receive(0,d,3,0,ms++);
        for(int i=0;i<36;++i)assert(!e.surfaceHeld(i));
    }
    std::cout<<"PASS host GOLDEN 36 buttons, duplicates, CC domain, unmapped notes, no physical Thru\n";
}
