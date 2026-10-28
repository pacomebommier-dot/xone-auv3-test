#pragma once
// Native S7 GROUP/LONG gate. Never guesses a physical button role.
// The physical gesture resolver must certify source ownership and retain
// the captured NAV revision. No synth/backend/profile ID is needed.
#include "../Core/NavS3.hpp"
#include <cstdint>
namespace xone::host {
struct QualifiedGroupAction {
    bool fill{};
    unsigned page{},action{},physical{};
    std::uint64_t navRevision{};
    bool sourceOwned{}; // evidence from S2/S3, not an inferred route
};
class GroupActionGate {
public:
    static bool authorize(const l01::NavS3& nav,const QualifiedGroupAction& q,
                          bool physicalHeld,bool navTransactionPending) noexcept {
        if(!q.sourceOwned || !physicalHeld || navTransactionPending ||
           !nav.stable()||nav.hasStage() || nav.revision()!=q.navRevision ||
           q.physical>=36 || q.page>=8 || q.action>=(q.fill?6u:3u))
            return false;
        const auto& n=nav.active();
        // Only the committed root-1/YELLOW surface may produce these S7
        // actions; overlays, PRESET and RESET own the same physical keys.
        if(n.at(291)!=1 || n.at(292)!=0 || n.contextMode!=0 || n.presetFlag!=0)return false;
        // The two instruments have separate, captured selected pages.
        const int selected=n.at(q.fill?298:294);
        if(selected!=static_cast<int>(q.page+1))return false;
        if(q.fill){
            // FILL page must be active, not a hidden selection under YELLOW.
            if(n.at(304)!=1)return false;
        }else if(n.at(304)!=0)return false;
        return true;
    }
};
} // namespace xone::host
