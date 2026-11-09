#include "../Shared/XoneMuteBarBridge.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static GridBatch edge(bool bar){
 GridBatch x{};x.valid=true;x.count=1;x.events[0]={bar?GridKind::Bar:GridKind::Beat,12,bar?4:3};return x;
}
int main(){
 MuteBarBridge b;
 assert(!b.request(0,GlobalMuteState::Gesture::ShortMute,false));
 assert(!b.request(16,GlobalMuteState::Gesture::ShortMute,true));
 assert(!b.request(15,GlobalMuteState::Gesture::LongSolo,true));
 assert(b.request(0,GlobalMuteState::Gesture::ShortMute,true));
 assert(b.request(1,GlobalMuteState::Gesture::ShortMute,true));
 assert(b.observe(edge(false))==0&&!b.state().explicitlyMuted(0));
 assert(b.observe(edge(true))==1&&b.state().explicitlyMuted(0)&&b.state().explicitlyMuted(1));
 assert(b.request(2,GlobalMuteState::Gesture::LongSolo,true));
 GridBatch absent{};assert(b.observe(absent)==0);
 assert(b.state().soloTarget()==-1);
 assert(b.observe(edge(true))==1&&b.state().soloTarget()==2);
 assert(b.request(3,GlobalMuteState::Gesture::ShortMute,true));
 b.stop();assert(b.observe(edge(true))==0&&b.state().soloTarget()==2);
 assert(b.request(2,GlobalMuteState::Gesture::LongSolo,true));
 b.restore();assert(b.observe(edge(true))==0&&b.state().soloTarget()==2);
 std::cout<<"PASS guarded MUTE PAGE BAR bridge and multiple BASE pending\n";
}
