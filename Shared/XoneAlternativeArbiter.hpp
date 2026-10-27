#pragma once
// E3/L05.4 — two independent INST/YELLOW pools, 2 XONE groups per pool.
// C01: later, separately arbitrated request may replace another PENDING owner.
// C03: within the same arbitration, FIRST RECEIVED wins.
// C04: inconsistent PRESET/RED recall is rejected as a whole.
// This is a control-ownership transaction, not an audio router or MIDI emitter.
#include <array>
#include <cstdint>
#include <limits>
namespace xone::host {
class AlternativeArbiter {
public:
    static constexpr int NoTarget=-1;
    using Targets=std::array<int,4>;
    using Ready=std::array<bool,4>;
    enum class Request : unsigned char { Accepted, SamePending, Cancelled, Rejected };
    struct Plan {
        Targets from{{-1,-1,-1,-1}};
        Targets to{{-1,-1,-1,-1}};
        std::uint64_t revision{};
        bool valid{}, recall{};
        // Callers must complete OFF/release duties for each from!=to
        // BEFORE calling finalize(plan,true) and emitting new destination ON.
    };
    const Targets& active() const noexcept {return active_;}
    const Targets& pending() const noexcept {return pending_;}
    std::uint64_t revision() const noexcept {return revision_;}
    static bool coherent(const Targets& targets) noexcept {
        for(unsigned i=0;i<4;++i)
            if(targets[i]<NoTarget || targets[i]>3)return false;
        return !(targets[0]>=0&&targets[0]==targets[1]) &&
               !(targets[2]>=0&&targets[2]==targets[3]);
    }
    Request request(unsigned owner,int target,std::uint64_t arbitration) noexcept {
        if(owner>=4 || target<0 || target>3 || arbitration==0 ||
           arbitration<lastArbitration_ ||
           revision_==std::numeric_limits<std::uint64_t>::max())return Request::Rejected;
        const unsigned other=owner^1u;
        if(active_[other]==target)return Request::Rejected; // occupied ACTIVE is never stolen
        if(target==active_[owner]) {
            lastArbitration_=arbitration;
            if(pending_[owner]<0)return Request::SamePending;
            pending_[owner]=-1;++revision_;return Request::Cancelled;
        }
        if(pending_[owner]==target) {
            lastArbitration_=arbitration;
            return Request::SamePending;
        }
        if(pending_[other]==target && arbitration<=pendingArbitration_[other])
            return Request::Rejected; // same arbitration: first received prevails
        // A later, separately arbitrated request supersedes only an uncommitted claim.
        if(pending_[other]==target)pending_[other]=-1;
        pending_[owner]=target;
        pendingArbitration_[owner]=arbitration;
        lastArbitration_=arbitration;
        ++revision_;
        return Request::Accepted;
    }
    // STOP before the BAR cancels pending requests; previously active owners remain.
    void stop() noexcept {clearPending();}
    void clearPending() noexcept {
        if(pending_!=Targets{{-1,-1,-1,-1}}){
            pending_.fill(-1);++revision_;
        }
    }
    Plan prepareNextBar(const Ready& baseValidated) const noexcept {
        Targets proposed=active_;
        for(unsigned i=0;i<4;++i)if(pending_[i]>=0)proposed[i]=pending_[i];
        return prepare(proposed,baseValidated,false);
    }
    // Full recall, not best-effort application of individual groups.
    Plan prepareRecall(const Targets& restored,const Ready& baseValidated) const noexcept {
        return prepare(restored,baseValidated,true);
    }
    // Call ONLY after the old destination owners and their OFF debts are dealt with.
    // false means that physical output could not be closed; logical state is not advanced.
    bool finalize(const Plan& plan,bool releaseConfirmed) noexcept {
        if(!releaseConfirmed||!plan.valid||plan.revision!=revision_||
           revision_==std::numeric_limits<std::uint64_t>::max())return false;
        active_=plan.to;pending_.fill(-1);++revision_;return true;
    }
private:
    Plan prepare(const Targets& next,const Ready& ready,bool recall) const noexcept {
        Plan p{};p.from=active_;p.to=next;p.revision=revision_;p.recall=recall;
        if(!coherent(next))return p;
        for(unsigned i=0;i<4;++i)
            if(next[i]>=0 && next[i]!=active_[i] && !ready[i])return p;
        p.valid=true;return p;
    }
    Targets active_{{-1,-1,-1,-1}};
    Targets pending_{{-1,-1,-1,-1}};
    std::array<std::uint64_t,4> pendingArbitration_{};
    std::uint64_t lastArbitration_{},revision_{1};
};
} // namespace xone::host
