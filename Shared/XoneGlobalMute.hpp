#pragma once
// L06.1 PAGE MUTE global state, in-memory and independent from local DRUMS/FILL mute.
// Contract CONCEPTION_PAGE_MUTE_P2_V1.4: 15 sources + FX global at index 15.
// Gestures are enqueued by the caller, committed only on a host-authenticated BAR.
// All output routing and LED feedback remain separate.
#include <array>
#include <cstdint>
namespace xone::host {
class GlobalMuteState {
public:
    static constexpr unsigned Sources=15, Fx=15, Targets=16;
    using Muted=std::array<bool,Targets>;
    enum class Gesture : unsigned char { ShortMute, LongSolo };
    struct Pending {bool valid{};Gesture gesture{};unsigned target{};};
    bool arm(unsigned target,Gesture gesture) noexcept {
        if(target>=Targets || (target==Fx && gesture==Gesture::LongSolo))return false;
        queued_={true,gesture,target};
        return true;
    }
    Pending pending()const noexcept {return queued_;}
    // STOP cancels changes waiting for BAR, but preserves the applied LIVE.
    void stop()noexcept{queued_.valid=false;}
    // RED restore must cancel old and RED-local pending actions, not re-arm them.
    void cancelForRestore()noexcept{queued_.valid=false;}
    bool onBar()noexcept {
        if(!queued_.valid)return false;
        const auto p=queued_;queued_.valid=false;
        if(p.gesture==Gesture::ShortMute){
            // Normal mute while SOLO is active abandons the snapshot, without restoration.
            if(solo_>=0){solo_=-1;haveSnapshot_=false;}
            explicitMute_[p.target]=!explicitMute_[p.target];
        }else if(solo_<0){
            beforeSolo_=explicitMute_;haveSnapshot_=true;solo_=static_cast<int>(p.target);
        }else if(solo_==static_cast<int>(p.target)){
            // Voluntary exit restores the initial pre-SOLO snapshot.
            if(haveSnapshot_)explicitMute_=beforeSolo_;
            solo_=-1;haveSnapshot_=false;
        }else{
            // Moving SOLO keeps the FIRST snapshot and all explicit mutes intact.
            solo_=static_cast<int>(p.target);
        }
        return true;
    }
    bool explicitlyMuted(unsigned target)const noexcept {return target<Targets&&explicitMute_[target];}
    bool effectivelyMuted(unsigned target,bool localMuted=false)const noexcept {
        if(target>=Targets)return true;
        if(target==Fx)return explicitMute_[Fx]; // SOLO never masks FX global
        return explicitMute_[target] || localMuted || (solo_>=0&&solo_!=static_cast<int>(target));
    }
    int soloTarget()const noexcept{return solo_;}
    bool hasSnapshot()const noexcept{return haveSnapshot_;}
    const Muted& explicitLayer()const noexcept{return explicitMute_;}
    bool restoreApplied(const Muted& layer,int solo,const Muted& before,bool snapshotValid)noexcept{
        if(solo<-1||solo>=static_cast<int>(Sources)||((solo>=0)!=snapshotValid))return false;
        explicitMute_=layer;beforeSolo_=before;solo_=solo;haveSnapshot_=snapshotValid;
        queued_.valid=false;return true;
    }
private:
    Muted explicitMute_{},beforeSolo_{};
    Pending queued_{};
    int solo_{-1};
    bool haveSnapshot_{};
};
} // namespace xone::host
