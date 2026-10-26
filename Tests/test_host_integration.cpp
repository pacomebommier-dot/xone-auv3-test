#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
int main() {
    HostEngine e;
    uint8_t down[]{0x9E,13,127}, up[]{0x8E,13,0};
    assert(e.navMode()==0);
    e.receive(0,down,3,0,0);
    e.receive(0,up,3,1764,40);
    assert(e.navMode()==1); // GOLDEN BOT2 SHORT -> PRESET
    e.receive(0,down,3,2205,50);
    e.advance(250,11025); // timer without new MIDI -> LONG
    assert(e.longNavPending());
    e.receive(0,up,3,11466,260);
    assert(!e.longNavPending() && e.navMode()==1); // restore captured NAV
    e.receive(0,down,3,12000,300);
    e.stop(12001);
    assert(e.surfaceHeld(1));
    e.advance(600,22050);
    e.receive(0,up,3,22051,601);
    assert(!e.surfaceHeld(1) && e.navMode()==1); // no synthetic SHORT after STOP
    // Transport sample position may rewind; gesture time remains independent.
    for(int i=0;i<1000;++i) {
        e.receive(0,down,3,0,700+2*i);
        e.receive(0,up,3,1,701+2*i);
    }
    assert(e.navMode()==1); // even number of toggles, trace UI may be full
    uint8_t other[]{0x9D,13,127};
    e.receive(0,other,3,0,3000);
    e.receive(1,down,3,0,3001);
    assert(!e.surfaceHeld(1));
    assert(e.observed()==-2);
    std::cout << "PASS host K2->S1->S2->S3 NAV SHORT/LONG/STOP, independent clock, bounded trace\n";
}
