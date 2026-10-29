#pragma once
// S7 A01_SPEC_API_MIDI_VIRTUELLE_V1.2:
// MUTE is virtual NOTE 528..543 (15 global sources + FX global).
// No audio path or plugin-specific CC is assumed. ACK means callback noErr,
// NOT external AUM/plugin confirmation.
#include "XoneGlobalMute.hpp"
#include <array>
#include <cstdint>
namespace xone::host {
class MuteMidiProjection {
public:
 struct Packet {
  bool valid{};
  unsigned target{};
  std::uint8_t bytes[3]{};
 };
 void invalidate() noexcept {uncertain_.fill(true);dirty_.fill(true);}
 void observe(const GlobalMuteState& state) noexcept {
  for(unsigned i=0;i<GlobalMuteState::Targets;++i){
   desired_[i]=state.effectivelyMuted(i)?1:0;
   if(uncertain_[i]||sent_[i]!=desired_[i])dirty_[i]=true;
   else dirty_[i]=false;
  }
 }
 Packet next(unsigned& cursor) const noexcept {
  while(cursor<GlobalMuteState::Targets){
   const unsigned i=cursor++;
   if(!dirty_[i])continue;
   const unsigned api=528u+i;
   const unsigned channel=6u+api/128u; // 0-based MIDI7..14
   return {true,i,{static_cast<std::uint8_t>((desired_[i]?0x90:0x80)+channel),
                   static_cast<std::uint8_t>(api%128u),
                   static_cast<std::uint8_t>(desired_[i]?127:0)}};
  }
  return {};
 }
 void confirm(unsigned target,bool callbackSucceeded) noexcept {
  if(target>=GlobalMuteState::Targets)return;
  if(!callbackSucceeded){uncertain_[target]=true;dirty_[target]=true;return;}
  uncertain_[target]=false;
  sent_[target]=desired_[target];dirty_[target]=false;
 }
 bool pending(unsigned target) const noexcept {
  return target<GlobalMuteState::Targets&&dirty_[target];
 }
private:
 std::array<int,GlobalMuteState::Targets> desired_{},sent_{};
 std::array<bool,GlobalMuteState::Targets> dirty_{},uncertain_{};
};
} // namespace xone::host
