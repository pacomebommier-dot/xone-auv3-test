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
        if(selected!=static_cast<int>(q.page+1))return false;
        // The S3 route itself must own exactly this SPOT position.
        // For YELLOW: SPOT1 h0..2 => kind3/family4.
        // For FILL: SPOT3 h6..8 => kind3/family6; SPOT4
        // h9..11 => kind1/family6. Other root1 positions are NOT group.
        if(q.physical<20||q.physical>=32)return false;
        const auto r=n.route(static_cast<int>(q.physical));
        if(r.address<0||r.position!=static_cast<int>(q.action))return false;
        const int kind=r.address/1024;
        const int family=(r.address%1024)/32;
        const int page=r.address%32;
        return family==(q.fill?6:4) && page==selected &&
            kind==(q.fill && q.action>=3?1:3);
    }
    // Derive a candidate solely from a verified S3 route and true K2 SPOT
    // position. This does not assert source ownership: it is still checked
    // against the captured physical DOWN epoch and the current S3 NAV.
    static bool derive(const l01::NavS3& nav,unsigned physical,
                       std::uint32_t downEpoch,QualifiedGroupAction& out) noexcept {
        if(!nav.stable()||nav.hasStage()||physical<20||physical>=32||downEpoch==0)return false;
        const auto& n=nav.active();
        const auto r=n.route(static_cast<int>(physical));
        if(r.address<0||r.position<0)return false;
        const int kind=r.address/1024, family=(r.address%1024)/32, page=r.address%32;
        const bool fill=family==6;
        if(!fill && !(kind==3&&family==4))return false;
        if(fill && !((kind==3&&r.position<3)||(kind==1&&r.position>=3&&r.position<6)))return false;
        if(page<1||page>8)return false;
        QualifiedGroupAction q{fill,static_cast<unsigned>(page-1),
            static_cast<unsigned>(r.position),physical,nav.revision(),downEpoch,true};
        if(!authorize(nav,q,true,false))return false;
        out=q;return true;
    }
};
} // namespace xone::host
