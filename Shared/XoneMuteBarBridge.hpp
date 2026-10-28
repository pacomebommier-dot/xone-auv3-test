#pragma once
#include "XoneGlobalMute.hpp"
#include "XoneHostScheduler.hpp"
namespace xone::host {
// Only SOURCE-qualified BASE events in the verified MUTE page are admitted.
// No implicit NAV mapping or MIDI mapping.
class MuteBarBridge {
public:
 bool request(unsigned base,GlobalMuteState::Gesture g,bool pageOwned) noexcept {
   if(!pageOwned||base>=GlobalMuteState::Targets)return false;
   if(!armed_){if(!schedule_.enqueue(1,0,ScheduleWhen::NextBar))return false;armed_=true;}
   return mute_.arm(base,g);
 }
 unsigned observe(const GridBatch& grid) noexcept {
   unsigned committed=0;
   schedule_.consume(grid,[&](DueAction due) noexcept {
     if(due.action.command==1&&mute_.onBar())++committed;
     armed_=false;
   });
   return committed;
 }
 void stop() noexcept {schedule_.stop();mute_.stop();armed_=false;}
 void restore() noexcept {schedule_.invalidate();mute_.cancelForRestore();armed_=false;}
 bool restoreApplied(const GlobalMuteState::Applied& image,bool cancelPending) noexcept {
   auto next=mute_;
   if(!next.restoreApplied(image.layer,image.solo,image.beforeSolo,image.hasSnapshot))return false;
   if(!cancelPending)for(unsigned i=0;i<mute_.pendingCount();++i){
     const auto p=mute_.pendingAt(i);if(!next.arm(p.target,p.gesture))return false;
   }
   mute_=next;
   if(cancelPending){schedule_.invalidate();armed_=false;}
   return true;
 }
 const GlobalMuteState& state() const noexcept{return mute_;}
private:
 HostActionScheduler schedule_{};
 GlobalMuteState mute_{};
 bool armed_{};
};
} // namespace xone::host
