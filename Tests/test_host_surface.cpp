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
    for(int cc=0;cc<128;++cc) {
        uint8_t d[]{0xBE,static_cast<uint8_t>(cc),64};
        assert(!e.receive(0,d,3,0,ms++).emit); // never Thru to LED/music
    }
    for(int n=0;n<128;++n) {
        if(xone::l01::SurfaceInput::physicalControl(n)>=0)continue;
        uint8_t d[]{0x9E,static_cast<uint8_t>(n),127};
        e.receive(0,d,3,0,ms++);
        for(int i=0;i<36;++i)assert(!e.surfaceHeld(i));
    }
    std::cout<<"PASS host GOLDEN 36 buttons, duplicates, CC domain, unmapped notes, no physical Thru\n";
}
