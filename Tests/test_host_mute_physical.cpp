#include "../Core/MutePhysicalGate.hpp"
#include <cassert>
#include <iostream>
using namespace xone::l01;
using namespace xone::host;
static GridBatch bar(){GridBatch g{};g.valid=true;g.count=1;g.events[0]={GridKind::Bar,10,4};return g;}
int main(){
 MutePhysicalGate gate;MuteBarBridge bridge;
 assert(!gate.beginBot2(0,false)&&!gate.visible());
 assert(gate.beginBot2(10,true));
 assert(!gate.beginBase(0,20,true)); // no overlay yet
 assert(gate.releaseBot2(100)&&gate.visible());
 assert(gate.beginBase(0,150,true));
 assert(!gate.beginBase(0,151,true)); // duplicate DOWN
 assert(gate.releaseBase(0,300,bridge));
 assert(bridge.state().pendingCount()==1&&!bridge.state().explicitlyMuted(0));
 assert(bridge.observe(bar())==1&&bridge.state().explicitlyMuted(0));
 assert(gate.beginBase(1,400,true));
 gate.advance(600); // 200ms qualifies long before UP
 assert(gate.releaseBase(1,610,bridge));
 assert(bridge.observe(bar())==1&&bridge.state().soloTarget()==1);
 assert(gate.beginBot2(700,true));
 gate.advance(900);
 assert(!gate.visible()); // visit underlying page
 assert(!gate.beginBase(3,901,true));
 assert(gate.releaseBot2(910)&&gate.visible());
 assert(gate.beginBase(2,1000,true));
 gate.stop();bridge.stop();
 assert(!gate.releaseBase(2,1100,bridge));
 assert(!gate.releaseBot2(1100));
 assert(bridge.state().soloTarget()==1);
 std::cout<<"PASS MUTE physical gate qualified BOT2, BASE, 200ms, BAR, STOP and ownership\n";
}
