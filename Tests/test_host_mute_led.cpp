#include "../Shared/XoneMuteLedView.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
int main(){
 GlobalMuteState state;
 auto l=MuteLedView::compose(state);
 for(auto x:l)assert(x.color==MuteLedColor::Green&&!x.blinking);
 assert(state.arm(0,GlobalMuteState::Gesture::ShortMute));
 l=MuteLedView::compose(state);assert(l[0].color==MuteLedColor::Red&&l[0].blinking);
 assert(l[1].color==MuteLedColor::Green&&!l[1].blinking);
 assert(state.onBar());l=MuteLedView::compose(state);
 assert(l[0].color==MuteLedColor::Red&&!l[0].blinking);
 assert(state.arm(1,GlobalMuteState::Gesture::LongSolo));
 l=MuteLedView::compose(state);assert(l[1].color==MuteLedColor::Yellow&&l[1].blinking);
 assert(state.onBar());l=MuteLedView::compose(state);
 assert(l[1].color==MuteLedColor::Yellow&&!l[1].blinking);
 assert(l[0].color==MuteLedColor::Red&&!l[0].blinking); // explicit MUTE survives SOLO
 assert(l[2].color==MuteLedColor::Off&&!l[2].blinking);
 assert(state.arm(2,GlobalMuteState::Gesture::LongSolo));
 l=MuteLedView::compose(state);assert(l[2].color==MuteLedColor::Yellow&&l[2].blinking);
 assert(state.onBar());assert(state.soloTarget()==2);
 assert(state.arm(2,GlobalMuteState::Gesture::LongSolo)); // voluntary exit restores
 l=MuteLedView::compose(state);
 assert(l[2].color==MuteLedColor::Green&&l[2].blinking);
 assert(state.onBar());l=MuteLedView::compose(state);
 assert(l[2].color==MuteLedColor::Green&&!l[2].blinking);
 assert(state.arm(15,GlobalMuteState::Gesture::ShortMute));
 l=MuteLedView::compose(state);assert(l[15].color==MuteLedColor::Red&&l[15].blinking);
 assert(state.onBar());l=MuteLedView::compose(state);
 assert(l[15].color==MuteLedColor::Red&&!l[15].blinking);
 std::cout<<"PASS M6 LEDs semantic color and blink, preserved MUTE, SOLO move/restore, FX\n";
}
