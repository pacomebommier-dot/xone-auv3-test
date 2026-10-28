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
    std::uint32_t physicalDownEpoch{}; // captured only on a true physical DOWN
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
        // Root1 YELLOW has two mutually exclusive selection flags:
        // 301=>low YELLOW1..4 (field295), 302=>high YELLOW5..8 (field296).
        // Root1 FILL uses field298 + active flag304. A hidden selection is
        // not an owner and must not produce a GROUP action.
        int selected=-1;
        if(q.fill){
            if(n.at(304)!=1 || n.at(301)!=0 || n.at(302)!=0)return false;
            selected=n.at(298);
        }else{
            if(n.at(304)!=0)return false;
            if(n.at(301)==1 && n.at(302)==0)selected=n.at(295);
            else if(n.at(302)==1 && n.at(301)==0)selected=n.at(296);
            else return false;
        }
        return selected==static_cast<int>(q.page+1);
    }
};
} // namespace xone::host
