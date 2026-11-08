// Diagnostic NAV characterization, NOT a SOC-01 musical BASE acceptance test.
// The diagnostic NAV TOUCH handler is intentionally different from the BASE contract.
// Build: c++ -std=c++17 -I. Tests/test_host_soc01_base.cpp -o /tmp/navdiag && /tmp/navdiag
#include "../Core/CoreCoordinator.hpp"
#include <cassert>
#include <iostream>
using namespace xone::l01;
static void edge(CoreCoordinator& c,int note,bool down,int ms){assert(c.onMidi({14,down?144:128,note,down?127:0},ms));}
static CoreCoordinator ready(){CoreCoordinator c;assert(c.initializeSourceDefaultNav());c.enableDiagnosticTouchNav(true);return c;}
int main(){
    // First press selects immediately; its UP has no captured owner.
    auto c=ready();const auto before=c.nativeNav().active();
    edge(c,36,true,100);const auto selected=c.nativeNav().active();
    assert(selected!=before && "first diagnostic NAV press selects");
    edge(c,36,false,150);
    assert(c.nativeNav().active()==selected && "first release leaves selection");
    // Repeated press captures the owner. Short UP clears flag, long UP retains it.
    edge(c,36,true,200);assert(c.nativeTouchOwner()==4);
    edge(c,36,false,250);assert(c.nativeTouchOwner()==-1);
    assert(c.nativeNav().active().at(299)==0 && "short reselect clears flag");
    edge(c,36,true,300);edge(c,36,false,320); // reselect and restore flag
    assert(c.nativeNav().active().at(299)==1);
    edge(c,36,true,400);assert(c.nativeTouchOwner()==4);
    edge(c,36,false,600);assert(c.nativeTouchOwner()==-1);
    assert(c.nativeNav().active().at(299)==1 && "long reselect preserves flag");
    std::cout << "PASS diagnostic NAV characterization (not SOC-01 BASE)\\n";
}
