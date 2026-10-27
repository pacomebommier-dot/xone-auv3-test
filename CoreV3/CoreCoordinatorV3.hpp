#pragma once
// Native V3 coordinator derived from the archived CoreCoordinator; historical source left intact.
// Physical events -> S1 surface -> S2 gestures -> qualified S3 INPUT QUEUE.
// No assumed musical mapping, routing, current-mode, or backend readiness.
// The previous Engine.hpp remains a separate structural oracle, not a musical owner.
#include "../Core/Engine.hpp"
#include "../Core/GestureS2.hpp"
#include "../Core/OutputS4.hpp"
#include "../Core/S3DecisionBoundary.hpp"
#include "../Core/NavS3.hpp"
#include "../Core/S3LongTransaction.hpp"
#include "../Core/S3TouchNav.hpp"
#include "../Core/LiveState.hpp"
#include "../Core/FxS6.hpp"
#include "../Core/BankS6.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::l01 {
class CoreCoordinatorV3 {
public:
    static constexpr std::size_t TraceCapacity=128;
    static constexpr std::size_t S3TraceCapacity=512;
    bool onMidi(MidiInput msg,std::int64_t ms)noexcept {
        if(ms<0||(started_&&ms<lastMs_))return false;
        if(msg.channel!=14||msg.value<0||msg.value>127||msg.number<0||msg.number>127)return false;
        if(count_+10>TraceCapacity||s3Count_+20>S3TraceCapacity)return false;
        // Stage *all* components, including surface; invalid input cannot leave a phantom Held.
        auto surf=surface_;auto gest=gesture_;auto s3=s3_;auto nav=nav_;auto tx=longTx_;auto touch=touch_;
        int machineLow=machineLastLow_,machineHigh=machineLastHigh_;
        const auto item=surf.accept(msg);
        GestureBatch b{};S3ProposalBatch proposals{};
        if(item.kind!=SurfaceKind::None){
            if(!gest.accept(item,ms,b)||!s3.accept(b,proposals,ms))return false;
        }
        if(!touch.process(b,ms,nav,s3,diagnosticTouch_,tx.pending() || touchOwner_!=-1))return false;
        if(!processQualifiedNav(nav,s3,tx,proposals,machineLow,machineHigh,touch.busy()))return false;
        surface_=surf;gesture_=gest;s3_=s3;nav_=nav;longTx_=tx;touch_=touch;machineLastLow_=machineLow;machineLastHigh_=machineHigh;
        append(b);appendS3(proposals);lastMs_=ms;started_=true;return true;
    }
    bool onTimer(std::int64_t ms)noexcept {
        if(ms<0||(started_&&ms<lastMs_)||count_+8>TraceCapacity||s3Count_+20>S3TraceCapacity)return false;
        auto gest=gesture_;auto s3=s3_;auto nav=nav_;auto tx=longTx_;auto touch=touch_;
        int machineLow=machineLastLow_,machineHigh=machineLastHigh_;
        GestureBatch b{};if(!gest.onTimer(ms,b))return false;
        S3ProposalBatch proposals{};if(!s3.accept(b,proposals,ms))return false;
        if(!touch.process(b,ms,nav,s3,diagnosticTouch_,tx.pending() || touchOwner_!=-1))return false;
        if(!processQualifiedNav(nav,s3,tx,proposals,machineLow,machineHigh,touch.busy()))return false;
        gesture_=gest;s3_=s3;nav_=nav;longTx_=tx;touch_=touch;machineLastLow_=machineLow;machineLastHigh_=machineHigh;
        append(b);appendS3(proposals);lastMs_=ms;started_=true;return true;
    }
    // SOURCE @z6; unlike observeS3NavForDiagnostic this supplies a validated full NAV.
    bool initializeSourceDefaultNav() noexcept {
        if(started_||nav_.stable())return false;
        NavS3 n=nav_;if(!n.initializeDefault())return false;
        auto s=s3_;if(!s.observeNav(n.active().at(292),true))return false;
        nav_=n;s3_=s;return true;
    }
    // Test-only gate; musical output and unqualified NAV transitions remain disabled.
    void enableDiagnosticShortNav(bool yes) noexcept {shortNavDiagnostic_=yes;}
    void enableNativeMuteBot2(bool yes) noexcept {muteBot2Exclusive_=yes;}
    void enableDiagnosticQualifiedNav(bool yes) noexcept {qualifiedNavDiagnostic_=yes;}
    // L01/07 legacy diagnostic override for unported TOUCH contexts.
    void enableDiagnosticTouchNav(bool yes) noexcept {diagnosticTouch_=yes;}
    // L01/09: one logical LIVE writer and distinct authenticated backend
    // observations. AUM source-port validation is a future adapter responsibility.
    void enableDiagnosticLive(bool yes) noexcept {diagnosticLive_=yes;}
    bool commitNativeLive(int address,int value,std::uint32_t expectedRevision,int verifiedProfile=-2) noexcept {
        return diagnosticLive_ && live_.commitDesired(address,value,expectedRevision,verifiedProfile);
    }
    bool commitNativeBrowserSpots(const BrowserSpotPlan& plan,std::uint32_t expectedRevision) noexcept {
        return diagnosticLive_ && live_.commitPlannedSpots(plan,expectedRevision);
    }
    bool restoreNativeSessionForDiagnostic(const LiveState::SessionImage& image) noexcept {
        return diagnosticLive_ && live_.restoreSession(image);
    }
    bool configureBackendObservationsForDiagnostic(int config,std::uint32_t port,
                     const std::array<LiveBinding,LiveState::BindingCount>& bindings) noexcept {
        if(!diagnosticLive_||!live_.configureBackend(config,port,bindings))return false;
        fx_.clearObserved();return true;
    }
    bool onBackendMidiForDiagnostic(std::uint32_t port,int channel,int command,int cc,int value,std::uint32_t epoch)noexcept {
        return diagnosticLive_ && live_.backendInput(port,channel,command,cc,value,epoch);
    }
    void backendDisconnectedForDiagnostic() noexcept {if(diagnosticLive_){live_.disconnectBackend();fx_.clearObserved();}}
    // L01/11: native volatile banks. Not a session serializer or an S8 bank.
    // No source SysEx emulation and no implicit MIDI output.
    bool initializeEmptyNativeBanksForDiagnostic() noexcept {
        return diagnosticLive_ && nativeBank_.initializeEmpty();
    }
    bool saveNativePatchForDiagnostic(int owner,int slot,int config,
            const BankS6::PatchImage& image,std::uint32_t expectedSlotRevision) noexcept {
        if(!diagnosticLive_)return false;
        return nativeBank_.savePatch(owner,slot,config,image,expectedSlotRevision);
    }
    bool saveNativeMemoryForDiagnostic(int owner,int slot,int config,
            const BankS6::MemoryImage& image,std::uint32_t expectedSlotRevision) noexcept {
        if(!diagnosticLive_)return false;
        auto nextBank=nativeBank_;
        if(!nextBank.saveMemory(owner,slot,config,image,expectedSlotRevision))return false;
        BankS6::MemoryRecord m{};
        if(!nextBank.readMemory(owner,slot,m))return false;
        FxS6::MemorySnapshot f{};
        f.supplied=true;f.valid=m.valid;f.config=m.config;f.revision=m.revision;f.values=m.values;
        auto nextFx=fx_;
        if(!nextFx.publishNativeMemory(owner,slot,f))return false;
        nativeBank_=nextBank;fx_=nextFx;return true;
    }
    // Explicit diagnostic recall of native MEMORY into the native single-writer
    // desired LIVE image. Source caller still has to qualify real gesture routing.
    bool recallNativeFxMemoryForDiagnostic(int owner,int slot,std::uint32_t expectedLiveRevision) noexcept {
        if(!diagnosticLive_||longTx_.pending()||touch_.busy())return false;
        BankS6::MemoryRecord m{};
        if(!nativeBank_.readMemory(owner,slot,m)||!m.valid)return false;
        auto next=live_;
        if(!next.commitFxMemoryImage(owner,m.values,expectedLiveRevision,m.config))return false;
        live_=next;return true;
    }
    const BankS6& nativeBank() const noexcept {return nativeBank_;}
    // L01/10: read-only FX-memory snapshots; no persistence implied.
    bool supplyFxMemoryForDiagnostic(int owner,int slot,const FxS6::MemorySnapshot& m)noexcept {
        return diagnosticLive_ && fx_.supplyMemory(owner,slot,m);
    }
    // Host-verified port token is required. This is a test-boundary shim,
    // not an actual AUv3 host-authenticated implementation.
    bool configureFxOriginForDiagnostic(std::uint32_t port)noexcept {
        if(!diagnosticLive_||port==0)return false;
        if(fxOrigin_!=port)fx_.clearObserved();
        fxOrigin_=port;return true;
    }
    bool onFxFrameForDiagnostic(std::uint32_t port,const std::array<int,20>& bytes)noexcept {
        if(!diagnosticLive_||port==0||port!=fxOrigin_)return false;
        FxEvent e{};if(!FxS6::decodeFrame(bytes,e))return false;
        return onFxEventForDiagnostic(port,e);
    }
    bool onFxEventForDiagnostic(std::uint32_t port,const FxEvent& e)noexcept {
        if(!diagnosticLive_||port==0||port!=fxOrigin_)return false;
        FxS6 nextFx=fx_;
        if(!nextFx.accept(e))return false;
        LiveState nextLive=live_;
        if(e.config!=fx_.config())nextLive.revokeFxObservations();
        const int a=FxS6::addressFor(e.owner,e.field);
        if(a>=0 && e.config>0){
            // Only a declared matching backend can contribute observed LIVE.
            nextLive.ingestFxObserved(port,e.config,a,e.known,e.value,e.epoch);
        }
        fx_=nextFx;live_=nextLive;return true;
    }
    bool refreshFxStatusesForDiagnostic(std::uint32_t port,const std::array<int,8>& bytes)noexcept {
        if(!diagnosticLive_||port==0||port!=fxOrigin_||!FxS6::isSnapshot(bytes))return false;
        return fx_.snapshot();
    }
    const FxS6& nativeFx()const noexcept{return fx_;}
    void drainFxStatusesForDiagnostic()noexcept{fx_.drain();}
    const LiveState& nativeLive()const noexcept{return live_;}
    BrowserSpotPlan planNativeBrowserSpots(const std::array<BrowserSpotInput,12>& spots)const noexcept {
        return live_.planBrowserSpots(spots);
    }
    int nativeTouchOwner() const noexcept {return touch_.physicalOwner();}
    S3TouchNav::Stats nativeTouchStats() const noexcept {return touch_.stats();}
    bool observeTouchOwnerForDiagnostic(int owner) noexcept {
        if(owner < -1 || owner > 35)return false;
        touchOwner_=owner;return true;
    }
    bool longNavPending() const noexcept {return longTx_.pending();}
    bool observeBrowserGuardForDiagnostic(bool b2,int a5,std::int64_t a6) noexcept {
        return s3_.observeBrowserGuardForDiagnostic(b2,a5,a6);
    }
    int longNavPhysicalOwner() const noexcept {return longTx_.owner();}
    bool stageNativeNav(const NavImage& i) noexcept {return !longTx_.pending() && nav_.stage(i);}
    bool commitNativeNav() noexcept {
        if(longTx_.pending())return false; // no retargeting while source h1=1
        NavS3 copy=nav_;if(!copy.commit())return false;
        auto s=s3_;if(!s.observeNav(copy.active().at(292),true))return false;
        nav_=copy;s3_=s;return true;
    }
    const NavS3& nativeNav() const noexcept{return nav_;}
    bool onHostStop()noexcept {
        if(!projection_.onHostStop())return false;
        live_.onHostStop();
        surface_.onHostStop();gesture_.onHostStop();s3_.onHostStop();longTx_.onHostStop();touch_.onHostStop();touchOwner_=-1;
        // The prototype has its own STOP contract and must be tested separately;
        // do not inject an unsequenced synthetic STOP into it.
        return true;
    }
    // Manual, explicitly guarded diagnostic S3->S4 boundary, not yet called by
    // normal S3 gesture dispatch. Actual logical mapping is still unqualified.
    bool projectQualifiedS3BatchForDiagnostic(const std::array<LogicalIntent,8>& intents,
              std::size_t n,int verifiedProfile,std::uint32_t expectedLiveRevision) noexcept {
        if(!diagnosticLive_||!qualifiedNavDiagnostic_||!projection_.enabled()||
           !nav_.stable()||longTx_.pending()||touch_.busy()||touchOwner_!=-1||
           nav_.hasStage()||live_.backendConfig()!=verifiedProfile||verifiedProfile<1||
           live_.revision()!=expectedLiveRevision)return false;
        return projection_.applyQualifiedBatch(intents,n);
    }
    bool applyLogicalOutput(LogicalIntent i)noexcept {return projection_.apply(i);}
    void enableTestMidiProjection(bool b)noexcept{projection_.setDiagnosticEnabled(b);}
    const SurfaceInput& surface()const noexcept{return surface_;}
    const GestureS2& gesture()const noexcept{return gesture_;}
    const S3DecisionBoundary& s3()const noexcept{return s3_;}
    bool observeS3NavForDiagnostic(int mode,bool validated) noexcept {return s3_.observeNav(mode,validated);}
    std::size_t s3TraceCount()const noexcept{return s3Count_;}
    S3Proposal s3Trace(std::size_t i)const noexcept{return i<s3Count_?s3Trace_[i]:S3Proposal{};}
    void drainS3Trace()noexcept{s3Count_=0;}
    const OutputS4& output()const noexcept{return projection_;}
    OutputS4& output()noexcept{return projection_;}
    const xone::Engine& originalStructuralPrototype()const noexcept{return structural_;}
    bool testStructuralEvent(xone::Event e)noexcept{return structural_.apply(e);}
    std::size_t traceCount()const noexcept{return count_;}
    GestureEvent trace(std::size_t i)const noexcept{return i<count_?trace_[i]:GestureEvent{};}
    void drainTrace()noexcept{count_=0;}
private:
    bool processQualifiedNav(NavS3& nav,S3DecisionBoundary& s3,S3LongTransaction& tx,
                             const S3ProposalBatch& proposals,int& machineLow,int& machineHigh,bool physicalTouchBusy) noexcept {
        if(!nav.stable())return true;
        for(std::size_t i=0;i<proposals.count;++i){
            const auto& p=proposals.items[i];
            const bool normal=p.kind==S3ProposalKind::NavShort||p.kind==S3ProposalKind::NavLong||p.kind==S3ProposalKind::NavLongRelease;
            const bool combo=p.kind==S3ProposalKind::Combo;
            if(p.kind==S3ProposalKind::ModeCycle){
                // @za depends on h1 but NOT the @zj a9 gate.
                if(qualifiedNavDiagnostic_ && !tx.pending() && p.navAtDown==7 && nav.active().at(292)==7 &&
                   nav.cycleMachineBank(machineLow,machineHigh))s3.observeNav(7,true);
                continue;
            }
            if(p.kind==S3ProposalKind::Dj2Rot){
                // @z4: DJ2+BOT3 qualifies @zj(h7=2) only outside 2/4 modes.
                if(qualifiedNavDiagnostic_ && !tx.pending() && touchOwner_==-1 && !physicalTouchBusy && !nav.hasStage() &&
                   nav.active().at(292)!=2 && nav.active().at(292)!=4 && p.navAtDown==nav.active().at(292) &&
                   nav.applyShortMode(2))s3.observeNav(nav.active().at(292),true);
                continue;
            }
            if(!normal&&!combo)continue;
            // BOT2 solo belongs exclusively to the native global-MUTE overlay.
            // Keep BOT2 combos and their qualified NAV roles independent.
            if(muteBot2Exclusive_ && p.role==1 && normal)continue;
            const int mode=NavS3::shortRoleMode(p.role);
            if(mode<0)continue;
            if(p.qualifiedPhase==QualifiedPhase::LongRelease){
                if(!qualifiedNavDiagnostic_ || !tx.pending() || !tx.release(nav,p.physical))continue;
                s3.observeNav(nav.active().at(292),true);
                continue;
            }
            // @zj guards h0=1, h1=0, a9=-1, plus context owned at true physical DOWN.
            if(tx.pending() || touchOwner_!=-1 || physicalTouchBusy || nav.hasStage() || p.navAtDown!=nav.active().at(292))continue;
            if(p.qualifiedPhase==QualifiedPhase::Short){
                if((shortNavDiagnostic_ || qualifiedNavDiagnostic_)&&nav.applyShortMode(mode))
                    s3.observeNav(nav.active().at(292),true);
            } else if(p.qualifiedPhase==QualifiedPhase::LongBegin){
                if(qualifiedNavDiagnostic_ && tx.begin(nav,p.physical,mode))
                    s3.observeNav(nav.active().at(292),true);
            }
        }
        return true;
    }
    void append(const GestureBatch& b)noexcept{for(std::size_t i=0;i<b.count;++i)trace_[count_++]=b.items[i];}
    void appendS3(const S3ProposalBatch& b)noexcept{for(std::size_t i=0;i<b.count;++i)s3Trace_[s3Count_++]=b.items[i];}
    SurfaceInput surface_{};
    GestureS2 gesture_{};
    S3DecisionBoundary s3_{};
    NavS3 nav_{};bool shortNavDiagnostic_{};bool qualifiedNavDiagnostic_{};bool muteBot2Exclusive_{};
    S3LongTransaction longTx_{};S3TouchNav touch_{};bool diagnosticTouch_{};int touchOwner_{-1};
    int machineLastLow_{-1},machineLastHigh_{-1};
    LiveState live_{};bool diagnosticLive_{};FxS6 fx_{};BankS6 nativeBank_{};std::uint32_t fxOrigin_{};
    OutputS4 projection_{};
    xone::Engine structural_{};
    std::array<GestureEvent,TraceCapacity> trace_{};
    std::array<S3Proposal,S3TraceCapacity> s3Trace_{};
    std::size_t s3Count_{};
    std::size_t count_{};
    std::int64_t lastMs_{};bool started_{};
};
}
