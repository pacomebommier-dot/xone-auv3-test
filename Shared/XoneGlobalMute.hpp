#pragma once
// L06.1: 15 musical sources and one FX-input target (BASE16).
// The stored layer never alters DRUMS/FILL local mutes or FX tails.
// Accepted commands are staged, and become effective ONLY at host BAR.
// Source: CONCEPTION_PAGE_MUTE_P2_V1.4 (Q7/Q8/M5/M6, 2026-09-30).
#include <array>
#include <cstdint>
namespace xone::host {
class GlobalMuteState {
public:
    static constexpr unsigned Sources=15, Fx=15, Targets=16;
    using Muted=std::array<bool,Targets>;
    enum class Gesture : unsigned char { ShortMute, LongSolo };
    struct Pending {bool valid{};Gesture gesture{};unsigned target{};};
    struct Applied {Muted layer{},beforeSolo{};int solo{-1};bool hasSnapshot{};};
    Applied applied()const noexcept {return {explicitMute_,beforeSolo_,solo_,haveSnapshot_};}
    Pending pendingAt(unsigned index)const noexcept {return index<count_?queued_[index]:Pending{};}
    // One pending action per BASE, at most 16 distinct targets in an arbitration.
    // Repeated arm on same BASE replaces its not-yet-applied action.
    bool arm(unsigned target,Gesture gesture) noexcept {
        if(target>=Targets || (target==Fx && gesture==Gesture::LongSolo))return false;
        for(unsigned i=0;i<count_;++i)if(queued_[i].target==target){
            queued_[i].gesture=gesture;return true;
        }
        if(count_==Targets)return false;
        queued_[count_++]={true,gesture,target};return true;
    }
    Pending pending() const noexcept {return count_?queued_[count_-1]:Pending{};}
    Pending pendingFor(unsigned target) const noexcept {
        if(target>=Targets)return {};
        for(unsigned i=0;i<count_;++i)if(queued_[i].target==target)return queued_[i];
        return {};
    }
    unsigned pendingCount() const noexcept {return count_;}
    void stop() noexcept {count_=0;}
    void cancelForRestore() noexcept {count_=0;}
    // All currently staged gestures belong to the SAME musical boundary.
    // FIFO between targets, replacement-in-place for a duplicate target.
    bool onBar() noexcept {
        if(!count_)return false;
        for(unsigned i=0,n=count_;i<n;++i){
            const auto p=queued_[i];
            if(p.gesture==Gesture::ShortMute){
                // Normal MUTE while SOLO is active drops the pre-SOLO snapshot.
                if(solo_>=0){solo_=-1;haveSnapshot_=false;}
                explicitMute_[p.target]=!explicitMute_[p.target];
            }else if(solo_<0){
                beforeSolo_=explicitMute_;haveSnapshot_=true;solo_=static_cast<int>(p.target);
            }else if(solo_==static_cast<int>(p.target)){
                if(haveSnapshot_)explicitMute_=beforeSolo_;
                solo_=-1;haveSnapshot_=false;
            }else{
                solo_=static_cast<int>(p.target); // retains initial snapshot
            }
        }
        count_=0;return true;
    }
    bool explicitlyMuted(unsigned target) const noexcept {return target<Targets&&explicitMute_[target];}
    bool effectivelyMuted(unsigned target,bool localMuted=false) const noexcept {
        if(target>=Targets)return true;
        if(target==Fx)return explicitMute_[Fx]; // no SOLO for FX global input
        return localMuted || (solo_>=0 ? solo_!=static_cast<int>(target) : explicitMute_[target]);
    }
    int soloTarget() const noexcept {return solo_;}
    bool hasSnapshot() const noexcept {return haveSnapshot_;}
    const Muted& explicitLayer() const noexcept {return explicitMute_;}
    bool restoreApplied(const Muted& layer,int solo,const Muted& before,bool snapshotValid) noexcept {
        if(solo<-1||solo>=static_cast<int>(Sources)||((solo>=0)!=snapshotValid))return false;
        explicitMute_=layer;beforeSolo_=before;solo_=solo;haveSnapshot_=snapshotValid;
        count_=0;return true;
    }
private:
    Muted explicitMute_{},beforeSolo_{};
    std::array<Pending,Targets> queued_{};
    unsigned count_{};
    int solo_{-1};
    bool haveSnapshot_{};
};
} // namespace xone::host
