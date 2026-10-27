#pragma once
// L06 source-qualified BOT2 (physical control 1) MUTE overlay.
// It does NOT change historical S3 NAV images or claim the MIDI/audio path.
// Short toggles overlay, long visits opposite context until genuine release.
// STOP freezes the current visible frame and disarms late UP.
namespace xone::l01 {
class MuteOverlayNav {
public:
    bool visible() const noexcept {return visible_;}
    bool momentary() const noexcept {return momentary_;}
    bool shortPress() noexcept {
        if(momentary_)return false;
        visible_=!visible_;return true;
    }
    bool longBegin() noexcept {
        if(momentary_)return false;
        original_=visible_;visible_=!visible_;momentary_=true;return true;
    }
    bool longRelease() noexcept {
        if(!momentary_)return false;
        visible_=original_;momentary_=false;return true;
    }
    void onHostStop() noexcept {momentary_=false;}
private:
    bool visible_{},momentary_{},original_{};
};
} // namespace xone::l01
