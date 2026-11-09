#pragma once
// L01/07: SOURCE L10 S3 @zj LONG / @ww / @z9 / @wv / @zi.
// Test-only port. No musical output, no assumption of valid real backend.
#include "NavS3.hpp"
namespace xone::l01 {
class S3LongTransaction {
public:
    bool begin(NavS3& nav,int physical,int mode) noexcept {
        if(pending_||physical<0||physical>35||mode<0||mode>8||!nav.stable()||nav.hasStage())return false;
        // Source @ww copies both active NAV and previous history before @zj commits.
        const NavImage before=nav.active(),history=nav.undoSnapshot();
        if(!nav.applyLongBeginMode(mode))return false;
        before_=before;history_=history;owner_=physical;pending_=true;
        return true;
    }
    bool release(NavS3& nav,int physical) noexcept {
        // Source @z9: only the same physical q3/h3 can release long ownership.
        if(!pending_||physical!=owner_||nav.hasStage())return false;
        if(!nav.restoreLong(before_,history_))return false;
        pending_=false;owner_=-1;
        return true;
    }
    void onHostStop() noexcept {
        // @z5 clears h1/h3 without generating UP or retroactive @z9.
        pending_=false;owner_=-1;
    }
    bool pending() const noexcept {return pending_;}
    int owner() const noexcept {return owner_;}
private:
    NavImage before_{},history_{};bool pending_{};int owner_{-1};
};
}
