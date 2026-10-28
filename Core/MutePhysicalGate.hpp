#pragma once
// Physical MUTE gesture gate. Source BOT2 owner and BASE capture only.
// This class is independent of legacy NAV; caller must explicitly grant BOT2.
// NOTE: qualification is transactional and STOP never fabricates release.
#include "MuteOverlayNav.hpp"
#include "../Shared/XoneMuteBarBridge.hpp"
#include <array>
#include <cstdint>
namespace xone::l01 {
class MutePhysicalGate {
public:
 void restoreVisible(bool visible)noexcept{nav_.restoreVisible(visible);}
 bool visible()const noexcept{return nav_.visible();}
 bool botHeld()const noexcept{return botHeld_;}
 bool ownsBase(unsigned base)const noexcept{return base<16&&baseHeld_[base];}
 bool timerNeeded()const noexcept {
  if(botHeld_&&!botLong_)return true;
  for(unsigned i=0;i<16;++i)if(baseHeld_[i]&&!baseLong_[i])return true;
  return false;
 }
 void cancelBot2()noexcept { // an S2 combo has captured BOT2 before LONG
  if(botLong_)nav_.longRelease();
  botHeld_=false;botLong_=false;
 }
 bool beginBot2(std::int64_t ms,bool qualifiedOwner)noexcept {
  if(!qualifiedOwner||botHeld_||ms<0)return false;
  botHeld_=true;botDown_=ms;botLong_=false;return true;
 }
 void advance(std::int64_t now)noexcept {
  if(botHeld_&&!botLong_&&now-botDown_>=200){botLong_=nav_.longBegin();}
  for(unsigned i=0;i<16;++i)if(baseHeld_[i]&&!baseLong_[i]&&now-baseDown_[i]>=200)
   baseLong_[i]=true;
 }
 bool releaseBot2(std::int64_t now)noexcept {
  if(!botHeld_||now<botDown_)return false;
  advance(now);
  botHeld_=false;
  return botLong_?nav_.longRelease():nav_.shortPress();
 }
 bool beginBase(unsigned base,std::int64_t ms,bool sourceOwned)noexcept {
  if(base>=16||!sourceOwned||!nav_.visible()||baseHeld_[base]||ms<0)return false;
  baseHeld_[base]=true;baseLong_[base]=false;baseDown_[base]=ms;return true;
 }
 bool releaseBase(unsigned base,std::int64_t now,xone::host::MuteBarBridge& bridge)noexcept {
  if(base>=16||!baseHeld_[base]||now<baseDown_[base])return false;
  advance(now);const bool longOne=baseLong_[base];
  baseHeld_[base]=false;baseLong_[base]=false;
  // DOWN owns this BASE; later overlay changes cannot retarget its UP.
  return bridge.request(base,longOne?xone::host::GlobalMuteState::Gesture::LongSolo:
    xone::host::GlobalMuteState::Gesture::ShortMute,true);
 }
 void stop()noexcept{
  nav_.onHostStop();
  botHeld_=false;
  // Ignore late real UP after STOP; continue retaining physical key ownership.
  for(unsigned i=0;i<16;++i)baseHeld_[i]=false;
 }
private:
 MuteOverlayNav nav_{};
 bool botHeld_{},botLong_{};
 std::int64_t botDown_{};
 std::array<bool,16> baseHeld_{},baseLong_{};
 std::array<std::int64_t,16> baseDown_{};
};
} // namespace xone::l01
