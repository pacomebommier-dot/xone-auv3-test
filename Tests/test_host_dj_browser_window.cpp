#include "../Shared/XoneDjBrowserWindow.hpp"
#include <cassert>
#include <iostream>
#include <cstdint>
using namespace xone::host;
int main(){
    DjBrowserWindow b;
    assert(!b.observeTurn(1,100));
    assert(!b.beginPush(100));
    b.configureDeck(3);assert(b.deck()==0);
    b.configureDeck(1);
    assert(b.observeTurn(1,100));
    assert(b.pendingAt(3099));
    assert(!b.pendingAt(3100));
    assert(b.observeTurn(-1,300));
    assert(b.pendingAt(3299)&&!b.pendingAt(3300));
    assert(b.beginPush(3299));
    assert(b.pushOwned());
    // Push stays owned after the window expires, until the true UP.
    assert(b.finishPush(3500)&&!b.pushOwned());
    assert(!b.pendingAt(3500));
    assert(!b.beginPush(3501));
    assert(b.observeTurn(1,4000));
    b.configureDeck(2);assert(!b.pendingAt(4001)); // no stale deck
    assert(b.observeTurn(1,4100));assert(b.beginPush(4101));
    b.stop();assert(!b.finishPush(4102)&&!b.pendingAt(4102));
    assert(!b.observeTurn(1,4099)); // regression in time rejected
    b.clear();b.configureDeck(1);
    assert(!b.observeTurn(1,INT64_MAX));
    assert(b.observeTurn(1,10)&&b.pendingAt(11));
    std::cout<<"PASS DJ1 window exact 3000ms, deck gate, refresh, owned push, STOP, monotonic clock\n";
}
