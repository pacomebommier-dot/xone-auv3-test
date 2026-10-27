#pragma once
// L11 PREPARED: semantic LED image for GLOBAL MUTE page only.
// K2 MIDI15 encoding belongs solely to the future S7 renderer.
// Q8/M6: fixed color = applied state, blinking color = armed NEXT_BAR target.
// Source CONCEPTION_PAGE_MUTE_P2_V1.4. No K2 messages are emitted here.
#include "XoneGlobalMute.hpp"
#include <array>
#include <cstdint>
namespace xone::host {
enum class MuteLedColor : std::uint8_t { Off=0, Red=1, Yellow=2, Green=3 };
struct MuteLed { MuteLedColor color{MuteLedColor::Off};bool blinking{}; };
class MuteLedView {
public:
 using Image=std::array<MuteLed,GlobalMuteState::Targets>;
 static MuteLed applied(const GlobalMuteState& mute,unsigned target) noexcept {
  if(target>=GlobalMuteState::Targets)return {};
  if(target==GlobalMuteState::Fx)
    return {mute.explicitlyMuted(target)?MuteLedColor::Red:MuteLedColor::Green,false};
  if(mute.soloTarget()==static_cast<int>(target))return {MuteLedColor::Yellow,false};
  if(mute.explicitlyMuted(target))return {MuteLedColor::Red,false};
  if(mute.soloTarget()>=0)return {MuteLedColor::Off,false};
  return {MuteLedColor::Green,false};
 }
 static Image compose(const GlobalMuteState& mute) noexcept {
  Image view{};
  GlobalMuteState proposed=mute;
  if(mute.pendingCount())proposed.onBar();
  for(unsigned i=0;i<view.size();++i){
   const bool awaiting=mute.pendingFor(i).valid;
   view[i]=awaiting?applied(proposed,i):applied(mute,i);
   view[i].blinking=awaiting;
  }
  return view;
 }
};
} // namespace xone::host
