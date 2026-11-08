// SOC-01: isolated SOCLE BASE short-toggle / long-momentary regression.
#include "../CoreV3/CoreCoordinatorV3.hpp"
#include <cassert>
#include <iostream>
using namespace xone::l01;
static void edge(CoreCoordinatorV3& c,bool down,int ms) {
    assert(c.onMidi({14,down?144:128,36,down?127:0},ms));
}
int main(){
    CoreCoordinatorV3 c;
    assert(c.initializeSourceDefaultNav());
    c.enableDiagnosticTouchNav(true);
    const NavImage initial=c.nativeNav().active();
    assert(initial.at(292)==0);
    edge(c,true,100);assert(!(c.nativeNav().active()==initial));
    const NavImage selected=c.nativeNav().active();
    edge(c,false,150);assert(c.nativeNav().active()==selected); // short ON
    edge(c,true,200);assert(c.nativeNav().active()==initial);
    edge(c,false,250);assert(c.nativeNav().active()==initial); // short OFF
    edge(c,true,300);assert(c.nativeNav().active()==selected);
    edge(c,false,500);assert(c.nativeNav().active()==initial); // long ON -> restore
    edge(c,true,600);edge(c,false,650);assert(c.nativeNav().active()==selected);
    edge(c,true,700);assert(c.nativeNav().active()==initial);
    edge(c,false,900);assert(c.nativeNav().active()==selected); // long OFF -> restore
    std::cout<<"PASS SOC-01 SOCLE BASE four gestures\\n";
}
