// SOC-01: actual HostEngine routing, not frozen diagnostic NAV assertions.
// SOCLE BASE: short toggles, long restores the prior musical route, STOP cancels.
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void base(HostEngine& e,bool down,int ms){
    const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),36,static_cast<uint8_t>(down?127:0)};
    assert(!e.receive(0,b,3,ms,ms).emit);
}
int main(){
    HostEngine e;
    assert(e.navMode()==0);
    const auto off=e.routeForControl(40); // CC4 musical route in the SOCLE source.
    base(e,true,10);
    const auto on=e.routeForControl(40);
    assert(!(on==off) && "BASE DOWN must select the source musical route");
    base(e,false,60);
    assert(e.routeForControl(40)==on && "short from OFF commits ON");
    base(e,true,100);
    assert(e.routeForControl(40)==off && "BASE DOWN must toggle ON to OFF");
    base(e,false,150);
    assert(e.routeForControl(40)==off && "short from ON commits OFF");
    base(e,true,200);
    assert(e.routeForControl(40)==on);
    base(e,false,400);
    assert(e.routeForControl(40)==off && "long from OFF restores OFF at 200ms");
    base(e,true,450);base(e,false,500);
    assert(e.routeForControl(40)==on);
    base(e,true,600);
    assert(e.routeForControl(40)==off);
    base(e,false,800);
    assert(e.routeForControl(40)==on && "long from ON restores ON at 200ms");
    base(e,true,900);
    assert(e.routeForControl(40)==off);
    e.stop(901); // Cancel an in-flight gesture; no fake UP.
    assert(e.routeForControl(40)==on && "STOP restores the previous committed selection");
    base(e,false,910);
    assert(e.routeForControl(40)==on && "late UP after STOP is inert");
    // The existing test_host_musical_routes exercises real CC4 -> API715.
    assert(e.navMode()==0);
    // Exhaustively exercise all 16 BASE notes, both header families (3/5)
    // and both short/long transitions using the true V3 coordinator.
    for(int note=36;note<52;++note){
        xone::l01::CoreCoordinatorV3 c;
        assert(c.initializeSourceDefaultNav());
        c.enableDiagnosticTouchNav(true);
        auto send=[&](bool down,int ms){
            assert(c.onMidi({14,down?144:128,note,down?127:0},ms));
        };
        auto selectedFlags=[&](){
            const auto& n=c.nativeNav().active();
            return n.at(299)+n.at(300)+n.at(303);
        };
        const auto initial=c.nativeNav().active();
        send(true,10);assert(selectedFlags()==1);
        const auto selected=c.nativeNav().active();
        send(false,60);assert(c.nativeNav().active()==selected); // short ON
        send(true,100);assert(selectedFlags()==0); // temporary OFF starts at DOWN
        send(false,150);assert(selectedFlags()==0); // short OFF
        send(true,200);assert(selectedFlags()==1);
        const auto onAgain=c.nativeNav().active();
        send(false,400);assert(selectedFlags()==0); // long from OFF restores OFF
        send(true,450);send(false,500);assert(selectedFlags()==1);
        const auto onBeforeLong=c.nativeNav().active();
        send(true,600);assert(selectedFlags()==0);
        send(false,800);assert(c.nativeNav().active()==onBeforeLong); // long from ON
        send(true,900);assert(selectedFlags()==0);
        assert(c.onHostStop());
        assert(c.nativeNav().active()==onBeforeLong); // STOP cancels pending
        send(false,910);assert(c.nativeNav().active()==onBeforeLong);
        assert(!(selected==initial) && !(onAgain==initial));
        xone::l01::CoreCoordinatorV3 firstLong;
        assert(firstLong.initializeSourceDefaultNav());
        firstLong.enableDiagnosticTouchNav(true);
        const auto before=firstLong.nativeNav().active();
        assert(firstLong.onMidi({14,144,note,127},10));
        assert(firstLong.onMidi({14,128,note,0},210));
        assert(firstLong.nativeNav().active()==before); // initial long with no previous short
    }
    std::cout<<"PASS SOC-01 SOCLE 16 BASE gestures, true UP, STOP and musical routes\n";
}
