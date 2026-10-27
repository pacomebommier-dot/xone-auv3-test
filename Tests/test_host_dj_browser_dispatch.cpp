#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static Result edge(HostEngine& e,int note,bool down,int ms){
    const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),
        static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};
    return e.receive(0,b,3,ms*48,ms);
}
static Result turn(HostEngine& e,int raw,int ms){
    const uint8_t b[]{0xBE,20,static_cast<uint8_t>(raw)};
    return e.receive(0,b,3,ms*48,ms);
}
int main(){
    // No fabricated deck binding in the AU: relative MIDI remains available,
    // but it does not acquire BOT2 until the separate backend seam is ready.
    {
        HostEngine e;e.enableNativeMutePage(true);
        auto r=turn(e,1,100);assert(r.emit);e.sent(r,0);
        assert(!e.djBrowserPendingAt(101));
        edge(e,13,true,150);edge(e,13,false,170);
        assert(e.mutePageVisible()&&e.browserLoadIntentsForDiagnostic()==0);
    }
    // One actual, acknowledged CC51 makes the window. Failed callback never does.
    {
        HostEngine e;assert(e.configureDjBrowserDeckForDiagnostic(1));
        e.enableNativeMutePage(true);
        auto r=turn(e,1,100);assert(r.emit);e.sent(r,-1);
        assert(!e.djBrowserPendingAt(101));
        edge(e,13,true,200);edge(e,13,false,240);
        assert(e.mutePageVisible()&&e.browserLoadIntentsForDiagnostic()==0);
    }
    // Browser wins against BOT2/MUTE, including expiry between DOWN and UP.
    {
        HostEngine e;assert(e.configureDjBrowserDeckForDiagnostic(2));
        e.enableNativeMutePage(true);
        auto r=turn(e,1,100);assert(r.emit);e.sent(r,0);
        assert(e.djBrowserPendingAt(3099)&&!e.djBrowserPendingAt(3100));
        r=turn(e,127,300);assert(r.emit);e.sent(r,0);
        assert(e.djBrowserPendingAt(3299)&&!e.djBrowserPendingAt(3300));
        edge(e,13,true,3299);
        assert(e.djBrowserOwnsBot2()&&!e.mutePageVisible());
        e.advance(3500,3500*48);
        edge(e,13,false,3501);
        assert(!e.djBrowserOwnsBot2()&&!e.mutePageVisible());
        assert(e.browserLoadIntentsForDiagnostic()==1&&e.lastBrowserLoadDeckForDiagnostic()==2);
        edge(e,13,true,3600);edge(e,13,false,3620);
        assert(e.mutePageVisible()&&e.browserLoadIntentsForDiagnostic()==1);
    }
    // Browser owner23 cannot be stolen by a BOT combo after the captured DOWN.
    {
        HostEngine e;assert(e.configureDjBrowserDeckForDiagnostic(1));
        e.enableNativeMutePage(true);
        auto r=turn(e,1,10);e.sent(r,0);
        edge(e,13,true,20);assert(e.djBrowserOwnsBot2());
        edge(e,15,true,30);edge(e,15,false,40);
        edge(e,13,false,50);
        assert(!e.mutePageVisible()&&e.navMode()==0);
        assert(e.browserLoadIntentsForDiagnostic()==1);
    }
    // STOP cancels the captured owner and does not replay a LOAD at late real UP.
    {
        HostEngine e;assert(e.configureDjBrowserDeckForDiagnostic(1));
        e.enableNativeMutePage(true);
        auto r=turn(e,1,10);e.sent(r,0);
        edge(e,13,true,20);assert(e.djBrowserOwnsBot2());
        e.stop(30*48);edge(e,13,false,40);
        assert(!e.djBrowserOwnsBot2()&&e.browserLoadIntentsForDiagnostic()==0);
        assert(!e.mutePageVisible());
    }
    // A stale, formerly prepared CC51 after STOP never re-arms Browser.
    {
        HostEngine e;assert(e.configureDjBrowserDeckForDiagnostic(1));
        auto r=turn(e,1,10);assert(r.emit);
        e.stop(11*48);assert(!e.canSend(r));
        e.sent(r,0);assert(!e.djBrowserPendingAt(20));
    }
    // Missing MIDI output cannot authorize Browser just because a packet
    // was prepared; BOT2 keeps its original MUTE owner.
    {
        HostEngine e;assert(e.configureDjBrowserDeckForDiagnostic(1));
        e.enableNativeMutePage(true);
        auto r=turn(e,1,10);assert(r.emit);
        e.noOutput(r);
        assert(!e.djBrowserPendingAt(11));
        edge(e,13,true,20);edge(e,13,false,40);
        assert(e.mutePageVisible()&&e.browserLoadIntentsForDiagnostic()==0);
    }
    // Same relative delta twice is two browser navigation commands.
    // A known deck is necessary but insufficient until delivery succeeds.
    {
        HostEngine e;assert(e.configureDjBrowserDeckForDiagnostic(1));
        auto r=turn(e,1,10);assert(r.emit);e.sent(r,0);
        assert(e.djBrowserPendingAt(3009)&&!e.djBrowserPendingAt(3010));
        r=turn(e,1,100);assert(r.emit&&r.bytes[2]==1);e.sent(r,0);
        assert(e.djBrowserPendingAt(3099)&&!e.djBrowserPendingAt(3100));
        // No callback may report an earlier prepared packet after newer
        // gestures and silently rearm the browser deadline.
        r=turn(e,1,200);assert(r.emit);
        auto future=turn(e,1,210);assert(future.emit);
        e.sent(r,0);
        assert(!e.djBrowserPendingAt(3150));
        e.sent(future,0);
        assert(e.djBrowserPendingAt(3150));
    }
    std::cout<<"PASS DJ1 Browser -> S3 owner23 -> BOT2 exclusion, failed callback, exact expiry, combo, STOP, no fake LOAD\n";
}
