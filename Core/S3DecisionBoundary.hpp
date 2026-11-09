#pragma once
// L01/05 — strict subset of L10 S3 @z2 @z3 @z4 @si @z5, source 2026-10-26.
// No claim to full NAV @z7/@zg, LIVE @z0/@zc, scheduler @y4, or musical S3 mapping.
// Emits PROPOSALS only; never outputs MIDI or mutates NAV/LIVE/patch banks.
// The context is an explicit observation, not an inferred profile/default.
#include "GestureS2.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::l01 {
enum class S3ProposalKind : std::uint8_t {
    NavShort, NavLong, NavLongRelease, ModeCycle, Combo, RedShort, RedLong, RedRelease, Dj2Rot
};
struct S3Proposal {
    S3ProposalKind kind{};int role{-1};int physical{-1};int navAtDown{-2};int navNow{-2};QualifiedPhase qualifiedPhase{QualifiedPhase::Short};
};
struct S3ProposalBatch {
    std::array<S3Proposal,20> items{};std::size_t count{};bool overflow{};
    bool push(S3Proposal x) noexcept {
        if(count>=items.size()){overflow=true;return false;}
        items[count++]=x;return true;
    }
};
class S3DecisionBoundary {
public:
    // Nav mode comes from a verified S3 snapshot, never synthesized from MIDI/LED.
    // Values 0,3,7 are exercised by this L01/05 diagnostic subset.
    bool observeNav(int mode, bool validated) noexcept {
        if(!validated||mode<0||mode>8)return false;
        nav_=mode;navKnown_=true;return true;
    }
    void invalidateNav() noexcept {navKnown_=false;nav_=-2;}
    bool navKnown() const noexcept{return navKnown_;}
    int nav() const noexcept{return nav_;}
    bool held(int i) const noexcept {return i>=0&&i<36&&held_[static_cast<std::size_t>(i)];}
    int physicalOwner(int i) const noexcept {return i>=0&&i<36?owner_[static_cast<std::size_t>(i)]:-1;}
    bool pendingCombo() const noexcept {return pendingRole_>=0;}
    // SOURCE @z3 : physical BOT4 sees held SPOT 20..31 in mode 3/7.
    // Such an owner goes to @zb (not yet ported), never to normal NAV cycle.
    bool spotHeldForBrowser() const noexcept {
        for(int i=20;i<32;++i)if(held_[static_cast<std::size_t>(i)])return true;
        return false;
    }
    unsigned rejected() const noexcept{return rejected_;}
    unsigned ignored() const noexcept{return ignored_;}
    // Explicit source-state observation for @z3 owner23; no synthetic browser window.
    bool observeBrowserGuardForDiagnostic(bool b2,int a5,std::int64_t a6) noexcept {
        if(a5<0 || a5>2 || a6<0)return false;
        browserB2_=b2;browserA5_=a5;browserUntil_=a6;return true;
    }
    bool accept(const GestureBatch& incoming, S3ProposalBatch& proposals,std::int64_t now=-1) noexcept {
        if(incoming.count>incoming.items.size()||incoming.overflow||proposals.count+incoming.count>proposals.items.size())return false;
        // Transactional staging: invalid frame never changes held, owners or proposals.
        S3DecisionBoundary staged=*this;
        if(now>=0)staged.sourceNow_=now;
        S3ProposalBatch out{};
        for(std::size_t i=0;i<incoming.count;++i){
            if(!staged.apply(incoming.items[i],out)||out.overflow)return false;
        }
        if(proposals.count+out.count>proposals.items.size())return false;
        *this=staged;
        for(std::size_t i=0;i<out.count;++i)proposals.push(out.items[i]);
        return true;
    }
    // Source @z5: every pressed key remains physically down but cannot dispatch a stale gesture.
    void onHostStop() noexcept {
        pendingRole_=-1;pendingFirst_=-1;pendingSecond_=-1;
        for(int i=0;i<36;++i)if(held_[static_cast<std::size_t>(i)])owner_[static_cast<std::size_t>(i)]=99;
    }
private:
    static int second(int role) noexcept { // S3 @si decoded against its constants
        switch(role){case 4:return 2;case 5:return 1;case 6:return 3;
            case 7:return 3;case 8:return 0;case 9:return 0;default:return -1;}
    }
    static int first(int role) noexcept {
        switch(role){case 4:return 1;case 5:return 2;case 6:return 1;
            case 7:return 2;case 8:return 2;case 9:return 1;default:return -1;}
    }
    static bool valid(const GestureEvent& e) noexcept {
        if(e.kind==GestureEventKind::RawEdge)return e.control>=0&&e.control<36&&(e.value==0||e.value==1);
        if(e.kind==GestureEventKind::RawDj)return e.control>=20&&e.control<=21&&e.value>=0&&e.value<=127;
        if(e.kind==GestureEventKind::RawCc)return e.control>=0&&e.control<=19&&e.value>=0&&e.value<=127;
        if(e.kind==GestureEventKind::Qualified){
            const int p=static_cast<int>(e.phase);
            if(e.role<0||e.role>10||p<0||p>4)return false;
            if(p==3&&e.role<4)return false;
            if(p==4&&e.role!=1&&e.role!=2)return false;
            return true;
        }
        return false;
    }
    void proposal(S3ProposalBatch& out,S3ProposalKind kind,int role,int control,int captured,QualifiedPhase ph=QualifiedPhase::Short) const noexcept {
        out.push({kind,role,control,captured,navKnown_?nav_:-2,ph});
    }
    bool apply(const GestureEvent& e,S3ProposalBatch& out) noexcept {
        if(!valid(e))return false;
        if(e.kind==GestureEventKind::RawCc||e.kind==GestureEventKind::RawDj)return true;
        if(e.kind==GestureEventKind::RawEdge){
            const int c=e.control;
            if(e.value==1){
                if(held_[c]){++ignored_;return true;}
                held_[c]=true;
                downNav_[c]=navKnown_?nav_:-2;
                owner_[c]=-1;
                if(c<4){
                    owner_[c]=c;
                    if(c==1&&nav_==3&&navKnown_)owner_[c]=20;
                    else if(c==2&&nav_==7&&navKnown_)owner_[c]=20;
                    else if(c==3&&(nav_==3||nav_==7)&&navKnown_){
                        if(spotHeldForBrowser())owner_[c]=22;
                        else if(nav_==7)owner_[c]=21;
                    }
                    // @z3 @st/@zk/@y6 for non-BOT is outside current subset.
                    if(c==1 && browserB2_ && browserA5_>0 && browserA5_<3 && sourceNow_>=0 && sourceNow_<browserUntil_)
                        owner_[c]=23; // @z3: pending BROWSER has precedence over ordinary NAV
                    if(pendingRole_>=4 && pendingRole_<=9 && c==pendingSecond_){
                        const int f=pendingFirst_;
                        // Exact @z3 check: first still held and owner not consumed/blocked.
                        if(f>=0&&held_[f]&&downNav_[f]>=0&&downNav_[c]>=0&&owner_[f]!=23&&owner_[f]!=24&&owner_[f]!=40&&owner_[f]!=99){
                            owner_[f]=30;owner_[c]=31;comboRole_[c]=pendingRole_;
                        }
                    }
                    // @z3 clears transient j9/j5/k0 at any BOT physical DOWN.
                    pendingRole_=-1;pendingFirst_=-1;pendingSecond_=-1;
                }
            }else{
                if(!held_[c]){++ignored_;return true;}
                const int prev=owner_[c];
                if(c==1 && prev==20 && downNav_[c]==3 && navKnown_)
                    proposal(out,S3ProposalKind::NavShort,1,c,downNav_[c]);
                else if(c==2 && prev==20 && downNav_[c]==7 && navKnown_)
                    proposal(out,S3ProposalKind::NavShort,2,c,downNav_[c]);
                else if(c==3 && prev==21 && navKnown_ && nav_==7)
                    proposal(out,S3ProposalKind::ModeCycle,3,c,downNav_[c]);
                // No unsourced SPOT, TOP, PRESET or PATCH output from remaining raw UPs.
                held_[c]=false;owner_[c]=-1;downNav_[c]=-2;comboRole_[c]=-1;
            }
            return true;
        }
        const int role=e.role;
        const auto phase=e.phase;
        if(phase==QualifiedPhase::ComboDown){
            if(role==10){
                if(held_[2]&&owner_[2]!=23&&owner_[2]!=24&&owner_[2]!=30&&owner_[2]!=31&&owner_[2]!=99){
                    owner_[2]=40;
                    if(navKnown_)proposal(out,S3ProposalKind::Dj2Rot,10,2,downNav_[2]);
                }
            }else{
                pendingRole_=role;pendingFirst_=first(role);pendingSecond_=second(role);
            }
            return true;
        }
        if(phase==QualifiedPhase::ShiftClosed)return true; // @z4 no NAV action.
        if(role==10)return true; // @z4: no release dispatch for DJ2 role10.
        if(role==0){
            if(held_[0]&&owner_[0]==0&&navKnown_&&downNav_[0]>=0){
                const auto k=phase==QualifiedPhase::Short?S3ProposalKind::RedShort:
                    (phase==QualifiedPhase::LongBegin?S3ProposalKind::RedLong:S3ProposalKind::RedRelease);
                proposal(out,k,0,0,downNav_[0]);
            }
            return true;
        }
        const int physical=role<4?role:second(role);
        if(physical<0 || !held_[physical] || !navKnown_ || downNav_[physical]<0) {++ignored_;return true;}
        if(role<4){if(owner_[physical]!=physical){++ignored_;return true;}}
        else if(owner_[physical]!=31||comboRole_[physical]!=role){++ignored_;return true;}
        // @zj/@z9 only invoked once physical ownership and capture are accepted.
        // NAV validation/commit is *not* done; proposal is intentionally inert.
        const auto k=phase==QualifiedPhase::Short?S3ProposalKind::NavShort:
            phase==QualifiedPhase::LongBegin?S3ProposalKind::NavLong:S3ProposalKind::NavLongRelease;
        proposal(out,role<4?k:S3ProposalKind::Combo,role,physical,downNav_[physical],phase);
        return true;
    }
    std::array<bool,36> held_{};
    std::array<int,36> owner_=[]{std::array<int,36>a{};a.fill(-1);return a;}();
    std::array<int,36> downNav_=[]{std::array<int,36>a{};a.fill(-2);return a;}();
    std::array<int,36> comboRole_=[]{std::array<int,36>a{};a.fill(-1);return a;}();
    bool navKnown_{};int nav_{-2};
    int pendingRole_{-1},pendingFirst_{-1},pendingSecond_{-1};
    bool browserB2_{};int browserA5_{};std::int64_t browserUntil_{};
    std::int64_t sourceNow_{-1};
    unsigned rejected_{},ignored_{};
};
}
