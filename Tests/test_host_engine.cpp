#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
int main() {
    HostEngine e;
    for (unsigned v=0; v<128; ++v) {
        for (unsigned cc : {4u,8u}) {
            uint8_t input[]{0xB0, static_cast<uint8_t>(cc), static_cast<uint8_t>(v)};
            auto r=e.receive(0,input,3,1000+v);
            assert(r.emit && r.bytes[0]==0xB0 && r.bytes[1]==(cc==4?74:71) && r.bytes[2]==v);
            e.sent(r,0);
            Trace t{};
            assert(e.pop(t) && t.stage==Stage::RxIn);
            assert(e.pop(t) && t.stage==Stage::Decision && t.reason==Reason::Mapped);
            assert(e.pop(t) && t.stage==Stage::TxOut && t.sample==1000+v);
            assert(!e.pop(t));
        }
    }
    uint8_t echo[]{0xB0,74,127};
    assert(!e.receive(0,echo,3,2000).emit); // a returned output cannot remap again
    uint8_t cc[]{0xB0,4,127};
    assert(!e.receive(1,cc,3,2001).emit); // untrusted cable, no observation
    assert(e.observed()==-2);
    for (uint8_t ch=1;ch<16;++ch) {
        uint8_t other[]{static_cast<uint8_t>(0xB0|ch),4,127};
        assert(!e.receive(0,other,3,2002).emit);
    }
    uint8_t down[]{0x9E,36,127}, up[]{0x9E,36,0};
    e.receive(0,down,3,3000); assert(e.surfaceHeld(4));
    e.stop(3001); assert(e.surfaceHeld(4));
    e.receive(0,up,3,3002); assert(!e.surfaceHeld(4));
    uint8_t bad[]{0xB0,4,128};
    assert(!e.receive(0,bad,3,4000).emit);
    assert(!e.receive(0,cc,2,4000).emit);
    assert(!e.receive(0,nullptr,3,4000).emit);
    for(unsigned n=0;n<1000;++n) assert(e.receive(0,cc,3,5000+n).emit);
    assert(e.dropped()>0); // slow UI must not block MIDI or grow memory
    assert(e.observed()==-2);
    Trace t{}; unsigned count=0; while(e.pop(t)) {++count; assert(count<=HostEngine::TraceCapacity);}
    assert(count>0);
    e.renderOverflow(6000);
    assert(e.overflows()==1);
    assert(e.pop(t) && t.stage==Stage::Overflow && t.sample==6000);
    std::cout << "PASS L02 CC values, trace order, origin gate, loop rejection, UNKNOWN, STOP/real UP, overflow\n";
}
