#include "../Shared/XoneGlobalMute.hpp"
#include <cassert>
#include <iostream>
using xone::host::GlobalMuteState;
int main(){
 GlobalMuteState m;
 assert(!m.arm(16,GlobalMuteState::Gesture::ShortMute));
 assert(!m.arm(15,GlobalMuteState::Gesture::LongSolo));
 assert(m.arm(0,GlobalMuteState::Gesture::ShortMute));
 assert(!m.explicitlyMuted(0)&&!m.effectivelyMuted(0));
 assert(m.onBar()&&m.explicitlyMuted(0));
 assert(m.arm(1,GlobalMuteState::Gesture::LongSolo));
 assert(m.onBar()&&m.soloTarget()==1&&m.hasSnapshot());
 assert(m.effectivelyMuted(0)&&!m.effectivelyMuted(1));
 assert(m.arm(2,GlobalMuteState::Gesture::LongSolo));
 assert(m.onBar()&&m.soloTarget()==2&&m.hasSnapshot());
 assert(m.arm(2,GlobalMuteState::Gesture::LongSolo));
 assert(m.onBar()&&m.soloTarget()==-1&&m.explicitlyMuted(0)); // same original snapshot
 assert(m.arm(1,GlobalMuteState::Gesture::LongSolo));assert(m.onBar());
 assert(m.arm(0,GlobalMuteState::Gesture::ShortMute));
 assert(m.onBar()&&m.soloTarget()==-1&&!m.explicitlyMuted(0)); // exit without restore
 assert(m.arm(15,GlobalMuteState::Gesture::ShortMute));assert(m.onBar());
 assert(m.explicitlyMuted(15)&&!m.effectivelyMuted(3));
 assert(m.arm(0,GlobalMuteState::Gesture::ShortMute));
 m.stop();assert(!m.pending().valid&&!m.onBar()&&!m.explicitlyMuted(0));
 assert(m.arm(0,GlobalMuteState::Gesture::ShortMute));
 m.cancelForRestore();assert(!m.onBar());
 assert(m.arm(4,GlobalMuteState::Gesture::LongSolo));assert(m.onBar());
 assert(m.effectivelyMuted(3)&&!m.effectivelyMuted(4));
 assert(m.effectivelyMuted(4,true)); // local mute untouched
 assert(m.explicitlyMuted(15)); // global FX is not a solo target
 GlobalMuteState::Muted explicitLayer{},snap{};
 explicitLayer[3]=true;snap[5]=true;
 assert(!m.restoreApplied(explicitLayer,15,snap,true));
 assert(!m.restoreApplied(explicitLayer,2,snap,false));
 assert(m.restoreApplied(explicitLayer,2,snap,true));
 assert(m.explicitlyMuted(3)&&m.soloTarget()==2);
 assert(m.arm(2,GlobalMuteState::Gesture::LongSolo));
 assert(m.onBar()&&m.soloTarget()==-1&&m.explicitlyMuted(5)); // saved pre-solo snapshot
 std::cout<<"PASS global mute/SOLO NEXT_BAR, exclusive snapshot, move, FX, STOP, RED cancel\n";
}
