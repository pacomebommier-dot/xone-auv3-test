#include "../Core/MuteOverlayNav.hpp"
#include <cassert>
#include <iostream>
using namespace xone::l01;
int main() {
 MuteOverlayNav n;
 assert(!n.visible()&&!n.momentary());
 assert(n.shortPress()&&n.visible());
 assert(n.shortPress()&&!n.visible());
 assert(n.longBegin()&&n.visible()&&n.momentary());
 assert(!n.shortPress()&&!n.longBegin());
 assert(n.longRelease()&&!n.visible());
 assert(n.shortPress()&&n.visible());
 assert(n.longBegin()&&!n.visible());
 assert(n.longRelease()&&n.visible());
 assert(n.longBegin()&&!n.visible());
 n.onHostStop();
 assert(!n.momentary()&&!n.visible());
 assert(!n.longRelease()); // no fabricated restore on true late UP
 assert(n.shortPress()&&n.visible());
 std::cout<<"PASS BOT2 overlay short/long/STOP, no navigation mutation\n";
}
