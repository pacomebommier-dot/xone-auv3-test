// SOC-01 characterization: standalone native reproducer (expected to FAIL until fixed).
// Build: c++ -std=c++17 -I. Tests/test_host_soc01_base.cpp -o /tmp/soc01 && /tmp/soc01
#include "../Core/CoreCoordinator.hpp"
#include <cassert>
#include <iostream>
using namespace xone::l01;
static void edge(CoreCoordinator& c,int note,bool down,int ms){assert(c.onMidi({14,down?144:128,note,down?127:0},ms));}
static CoreCoordinator ready(){CoreCoordinator c;assert(c.initializeSourceDefaultNav());c.enableDiagnosticTouchNav(true);return c;}
int main(){
    // BASE physical 4 = MIDI note 36; 200ms boundary.
    {
        auto c=ready();const auto before=c.nativeNav().active();
        edge(c,36,true,100);edge(c,36,false,150);
        assert(c.nativeNav().active()!=before && "OFF + short must latch ON");
    }
    {
        auto c=ready();const auto before=c.nativeNav().active();
        edge(c,36,true,100);edge(c,36,false,300);
        assert(c.nativeNav().active()==before && "SOC-01 OFF + long must restore OFF");
    }
    {
        auto c=ready();edge(c,36,true,0);edge(c,36,false,10);
        const auto before=c.nativeNav().active();
        edge(c,36,true,100);edge(c,36,false,150);
        assert(c.nativeNav().active()!=before && "ON + short must toggle OFF");
    }
    {
        auto c=ready();edge(c,36,true,0);edge(c,36,false,10);
        const auto before=c.nativeNav().active();
        edge(c,36,true,100);edge(c,36,false,300);
        assert(c.nativeNav().active()==before && "ON + long must restore ON");
    }
    std::cout << "PASS SOC-01 four transitions\n";
}
