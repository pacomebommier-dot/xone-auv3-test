#pragma once
// E3 DJ1 BROWSER: a bounded, monotonic, render-thread gesture gate.
// The owning backend must explicitly establish a valid deck binding.
// No track selection/load is sent by this module; a future S7 service
// adapter must consume the qualified request without fabricating an ACK.
#include <cstdint>
#include <limits>
namespace xone::host {
class DjBrowserWindow {
public:
    static constexpr std::int64_t WindowMs=3000;
    void configureDeck(int deck) noexcept {
        // Only the two established deck slots are accepted.
        if(deck!=1&&deck!=2){clear();return;}
        if(deck_!=deck)clear();
        deck_=deck;
    }
    bool observeTurn(int delta,std::int64_t now) noexcept {
        if(deck_<1||deck_>2||delta==0||now<0||
           now>std::numeric_limits<std::int64_t>::max()-WindowMs ||
           (clockKnown_&&now<lastMs_))return false;
        lastMs_=now;clockKnown_=true;
        deadline_=now+WindowMs;active_=true;
        return true;
    }
    bool pendingAt(std::int64_t now) const noexcept {
        return active_&&clockKnown_&&now>=lastMs_&&now<deadline_;
    }
    // A qualified BOT2 DOWN consumes the DJ1 browser gesture. A later
    // UP cannot trigger a second MUTE action even if time expires mid-hold.
    bool beginPush(std::int64_t now) noexcept {
        const bool owned=pendingAt(now);
        pushOwned_=owned;
        return owned;
    }
    bool finishPush(std::int64_t now) noexcept {
        const bool owned=pushOwned_;
        pushOwned_=false;
        if(owned&&now>=0&&(!clockKnown_||now>=lastMs_)){
            active_=false;lastMs_=now;clockKnown_=true;
        }
        return owned;
    }
    bool pushOwned() const noexcept {return pushOwned_;}
    int deck() const noexcept {return deck_;}
    std::int64_t deadlineMs() const noexcept {return active_?deadline_:0;}
    void stop() noexcept {active_=false;pushOwned_=false;deadline_=0;}
    void clear() noexcept {stop();deck_=0;clockKnown_=false;lastMs_=0;}
private:
    int deck_{};
    bool active_{},pushOwned_{},clockKnown_{};
    std::int64_t deadline_{},lastMs_{};
};
}
