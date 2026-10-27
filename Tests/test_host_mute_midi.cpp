#include "../Shared/XoneMuteMidiProjection.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static MuteMidiProjection::Packet take(MuteMidiProjection& p){
 unsigned cursor=0;return p.next(cursor);
}
int main(){
 GlobalMuteState m;MuteMidiProjection p;
 p.observe(m);assert(!take(p).valid);
 assert(m.arm(0,GlobalMuteState::Gesture::ShortMute));assert(m.onBar());
 p.observe(m);auto x=take(p);assert(x.valid&&x.target==0&&x.bytes[0]==0x9A&&x.bytes[1]==16&&x.bytes[2]==127);
 p.confirm(x.target,false);assert(p.pending(0)); // callback failed, must retry
 p.confirm(x.target,true);assert(!p.pending(0));
 assert(m.arm(0,GlobalMuteState::Gesture::ShortMute));assert(m.onBar());p.observe(m);
 x=take(p);assert(x.valid&&x.bytes[0]==0x8A&&x.bytes[1]==16&&x.bytes[2]==0);
 p.confirm(x.target,true);
 assert(m.arm(15,GlobalMuteState::Gesture::ShortMute));assert(m.onBar());p.observe(m);
 x=take(p);assert(x.valid&&x.target==15&&x.bytes[1]==31&&x.bytes[0]==0x9A);
 assert(m.arm(15,GlobalMuteState::Gesture::ShortMute));assert(m.onBar());p.observe(m);
 assert(!take(p).valid); // unsent transient value was superseded; no stale ON
 assert(m.arm(1,GlobalMuteState::Gesture::LongSolo));assert(m.onBar());p.observe(m);
 unsigned c=0;unsigned count=0;
 for(;;){auto q=p.next(c);if(!q.valid)break;
   assert(q.target!=1&&q.target!=15);
   assert(q.bytes[0]==0x9A);p.confirm(q.target,true);++count;
 }
 assert(count==14);
 assert(m.arm(1,GlobalMuteState::Gesture::LongSolo));assert(m.onBar());p.observe(m);
 c=0;count=0;for(;;){auto q=p.next(c);if(!q.valid)break;
   assert(q.bytes[0]==0x8A);p.confirm(q.target,true);++count;
 }
 assert(count==14);
 std::cout<<"PASS S7 MUTE NOTE 528..543, callback retry and SOLO projection\n";
}
