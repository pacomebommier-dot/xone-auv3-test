#pragma once
// Native V3 coordinator derived from the archived CoreCoordinator; historical source left intact.
// Physical events -> S1 surface -> S2 gestures -> qualified S3 INPUT QUEUE.
// No assumed musical mapping, routing, current-mode, or backend readiness.
// The previous Engine.hpp remains a separate structural oracle, not a musical owner.
#include "../Core/Engine.hpp"
#include "MachineFxAliases.hpp"
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
    bool onMidi(MidiInput msg,std::int64_t ms,bool muteBaseOwned=false)noexcept {
        if(ms<0||(started_&&ms<lastMs_))return false;
        if(msg.channel!=14||msg.value<0||msg.value>127||msg.number<0||msg.number>127)return false;
        if(count_+10>TraceCapacity||s3Count_+20>S3TraceCapacity)return false;
        // Stage *all* components, including surface; invalid input cannot leave a phantom Held.
        auto surf=surface_;auto gest=gesture_;auto s3=s3_;auto nav=nav_;auto tx=longTx_;auto touch=touch_;
        int machineLow=machineLastLow_,machineHigh=machineLastHigh_;
        auto save=localSave_;auto fxHold=machineFxHold_;bool fxView=machineFxBorrowed_;int fxMachine=machineFxViewMachine_;
        const auto item=surf.accept(msg);
        GestureBatch b{};S3ProposalBatch proposals{};
        if(item.kind!=SurfaceKind::None){
            if(!gest.accept(item,ms,b)||!s3.accept(b,proposals,ms))return false;
        }
        // SOC-01: SOCLE BASE header gestures are owned here, not by diagnostic TOUCH.
        // Capture original NAV at DOWN; short keeps the toggled state, long restores.
        auto socle=socleBase_;
        bool socleConsumed=false;
        if(!muteBaseOwned && diagnosticTouch_ && b.count==1){
            const auto& e=b.items[0];
            if(e.kind==GestureEventKind::RawEdge && e.control>=4 && e.control<20){
                if(e.value==1 && socle.physical<0 && nav.stable() && !nav.hasStage() &&
                   nav.active().at(292)==0 && !tx.pending() && !touch.busy() &&
                   s3.physicalOwner(1)==-1 && s3.physicalOwner(2)==-1 && s3.physicalOwner(3)==-1){
                    const auto route=nav.active().route(e.control);
                    const int group=(route.address%1024)/32;
                    const int page=nav.active().at(291);
                    if(route.address>=0 && route.address/1024==2 &&
                       (group==3+page || group==5+page)){
                        const NavImage before=nav.active();
                        NavImage next=before;
                        const int value=route.address%32;
                        int flag=-1,primary=-1;
                        if(group==3+page){
                            primary=293+2*page+(value>4?1:0);
                            flag=299+2*page+(value>4?1:0);
                        }else{primary=297+page;flag=303+page;}
                        const bool selected=before.at(flag)==1 && before.at(primary)==value;
                        next.at(299+2*page)=0;next.at(300+2*page)=0;next.at(303+page)=0;
                        if(!selected){next.at(flag)=1;next.at(primary)=value;}
                        if(NavS3::regenerate(next)&&NavS3::validate(next)&&nav.stage(next)&&nav.commit()){
                            socle={e.control,ms,before};
                            socleConsumed=true;
                            s3.observeNav(nav.active().at(292),true);
                        }
                    }
                }else if(e.value==0 && socle.physical==e.control){
                    if(ms>=socle.down && ms-socle.down>=200 && nav.stable() && !nav.hasStage()){
                        NavImage original=socle.before;
                        if(NavS3::regenerate(original)&&NavS3::validate(original)&&nav.stage(original)&&nav.commit())
                            s3.observeNav(nav.active().at(292),true);
                    }
                    socle={};socleConsumed=true;
                }
            }
        }
        if(!muteBaseOwned && !socleConsumed && !touch.process(b,ms,nav,s3,diagnosticTouch_,tx.pending() || touchOwner_!=-1))return false;
        // Source R02: BOT2 in SAMPLE and BOT3 in MACHINE are deferred
        // SAVE/NAV_RELEASE candidates. An already-qualified combo owns the
        // gesture instead; candidate never wins retrospectively.
        if(item.kind==SurfaceKind::Edge&&item.control<4){
            const int physical=item.control;
            if(item.phase==1){
                if(save.bot>=0&&physical!=save.bot)save={};
                if(save.bot<0 && !tx.pending() && !touch.busy() && nav.stable() &&
                   ((physical==1&&nav.active().at(292)==3&&s3.physicalOwner(1)==20) ||
                    (physical==2&&nav.active().at(292)==7&&s3.physicalOwner(2)==20)))
                    save={physical,nav.active().at(292),nav.revision(),false};
            }
        }
        if(!processQualifiedNav(nav,s3,tx,proposals,machineLow,machineHigh,touch.busy(),save.bot))return false;
        if(item.kind==SurfaceKind::Edge&&item.phase==0&&item.control==save.bot){
            // No TOP: BOT3 exits MACHINE to GLOBAL persistently, regardless
            // of hold duration. BOT2's SAMPLE->MUTE is managed in host layer.
            if(!save.consumed && save.mode==7 && nav.active().at(292)==7 &&
               nav.revision()==save.revision && !tx.pending() && !touch.busy()){
                if(!nav.applyShortMode(2)||!s3.observeNav(nav.active().at(292),true))return false;
            }
            save={};
        }
        if(nav.revision()!=nav_.revision()){
            updateMachineFxNavigation(nav_,nav,longTx_.pending(),tx.pending(),fxView,fxMachine);fxHold={};
        }
        else {
            if(fxHold.physical>=0&&surface_.held(static_cast<unsigned>(fxHold.physical)))qualifyMachineFxHold(fxHold,fxView,ms,nav,tx.pending());
            if(item.kind==SurfaceKind::Edge&&item.control>=4&&item.control<20){
                if(item.phase==1&&touch.physicalOwner()==item.control&&machineFxEligible(nav)){
                    const auto r=nav.active().route(item.control);
                    if(r.address==2*1024+2*32+nav.active().at(314)){fxHold={item.control,ms,nav.revision(),false};fxMachine=nav.active().at(314);}
                }else if(item.phase==0&&fxHold.physical==item.control)fxHold={};
            }
        }
        socleBase_=socle;machineFxHold_=fxHold;machineFxBorrowed_=fxView;machineFxViewMachine_=fxMachine;
        surface_=surf;gesture_=gest;s3_=s3;nav_=nav;longTx_=tx;touch_=touch;machineLastLow_=machineLow;machineLastHigh_=machineHigh;
        localSave_=save;
        append(b);appendS3(proposals);lastMs_=ms;started_=true;return true;
    }
    bool onTimer(std::int64_t ms)noexcept {
        if(ms<0||(started_&&ms<lastMs_)||count_+8>TraceCapacity||s3Count_+20>S3TraceCapacity)return false;
        auto gest=gesture_;auto s3=s3_;auto nav=nav_;auto tx=longTx_;auto touch=touch_;
        int machineLow=machineLastLow_,machineHigh=machineLastHigh_;
        GestureBatch b{};if(!gest.onTimer(ms,b))return false;
        S3ProposalBatch proposals{};if(!s3.accept(b,proposals,ms))return false;
        if(!touch.process(b,ms,nav,s3,diagnosticTouch_,tx.pending() || touchOwner_!=-1))return false;
        if(!processQualifiedNav(nav,s3,tx,proposals,machineLow,machineHigh,touch.busy(),localSave_.bot))return false;
        if(nav.revision()!=nav_.revision()){
            updateMachineFxNavigation(nav_,nav,longTx_.pending(),tx.pending(),machineFxBorrowed_,machineFxViewMachine_);machineFxHold_={};
        }
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
    // Backend state belongs to the host; S3 remains the sole NAV writer.
    void retainDjLoopPage(bool retain) noexcept {retainDjLoopPage_=retain;}
    bool djLoopStopPush() const noexcept {return djLoopStopPush_;}
    bool djLoopRotationAllowed() const noexcept {
        return nav_.stable() && !nav_.hasStage() && !longTx_.pending() &&
            touchOwner_==-1 && !touch_.busy() &&
            (nav_.active().at(292)==2 || nav_.active().at(292)==4);
    }
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
        const int index=BankS6::patchIndex(owner,slot);BankS6::PatchRecord prior{};
        if(!diagnosticLive_||index<0||!readNativePatch(owner,slot,prior)||prior.revision!=expectedSlotRevision||prior.revision>=BankS6::MaxRevision-1)return false;
        return nativeBank_.savePatch(owner,slot,config,image,expectedSlotRevision-patchRevisionBase_[static_cast<unsigned>(index)]);
    }
    bool saveNativeMemoryForDiagnostic(int owner,int slot,int config,
            const BankS6::MemoryImage& image,std::uint32_t expectedSlotRevision) noexcept {
        const int index=BankS6::memoryIndex(owner,slot);BankS6::MemoryRecord prior{};
        if(!diagnosticLive_||index<0||!readNativeMemory(owner,slot,prior)||prior.revision!=expectedSlotRevision||prior.revision>=BankS6::MaxRevision-1)return false;
        auto nextBank=nativeBank_;
        if(!nextBank.saveMemory(owner,slot,config,image,expectedSlotRevision-memoryRevisionBase_[static_cast<unsigned>(index)]))return false;
        BankS6::MemoryRecord m{};
        if(!nextBank.readMemory(owner,slot,m))return false;
        FxS6::MemorySnapshot f{};
        f.supplied=true;f.valid=m.valid;f.config=m.config;f.revision=m.revision+memoryRevisionBase_[static_cast<unsigned>(index)];f.values=m.values;
        auto nextFx=fx_;
        if(!nextFx.publishNativeMemory(owner,slot,f))return false;
        nativeBank_=nextBank;fx_=nextFx;return true;
    }
    bool captureNativeMemory(int owner,int slot) noexcept {
        const int index=BankS6::memoryIndex(owner,slot);
        if(!diagnosticLive_||index<0||index>=96||live_.sessionConfig()<1)return false;
        BankS6::MemoryImage image{};
        for(int f=0;f<9;++f){auto v=live_.desired(FxS6::addressFor(owner,f));
            if(!v.known)return false;
            image[static_cast<std::size_t>(f)]=v.value;}
        if(!BankS6::validateMemory(image))return false;
        if(!nativeBank_.initialized()&&!nativeBank_.initializeEmpty())return false;
        BankS6::MemoryRecord prior{};
        return readNativeMemory(owner,slot,prior)&&prior.revision<BankS6::MaxRevision-1&&
            nativeBank_.saveMemory(owner,slot,live_.sessionConfig(),image,prior.revision-memoryRevisionBase_[static_cast<unsigned>(index)]);
    }
    bool captureRhythmMemories(const std::array<int,12>& owners,unsigned count,int slot) noexcept {
        if(!diagnosticLive_||!count||count>owners.size()||live_.sessionConfig()<1)return false;
        auto staged=nativeBank_;
        if(!staged.initialized()&&!staged.initializeEmpty())return false;
        for(unsigned n=0;n<count;++n){
            const int owner=owners[n];
            const int index=BankS6::memoryIndex(owner,slot);
            if(index<0||index>=96)return false;
            BankS6::MemoryImage image{};
            for(int field=0;field<9;++field){
                const auto value=live_.desired(FxS6::addressFor(owner,field));
                if(!value.known)return false;
                image[static_cast<unsigned>(field)]=value.value;
            }
            BankS6::MemoryRecord prior{};
            if(!staged.readMemory(owner,slot,prior)||
               prior.revision+memoryRevisionBase_[static_cast<unsigned>(index)]>=BankS6::MaxRevision-1||
               !staged.saveMemory(owner,slot,live_.sessionConfig(),image,prior.revision))return false;
        }
        nativeBank_=staged;
        return true;
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
    // SAMPLE retains its source 40-field payload, including its own modes.
    // MACHINE snapshots require the S8 mode-excluding adapter, not this path.
    bool recallNativeSamplePatch(int owner,int slot,std::uint32_t bankRevision,
                                 std::uint32_t liveRevision) noexcept {
        if(!diagnosticLive_ || owner<0 || owner>=8 || longTx_.pending() ||
           touch_.busy() || nav_.hasStage() || touchOwner_!=-1 || liveRevision!=live_.revision())return false;
        BankS6::PatchRecord record{};
        if(!readNativePatch(owner,slot,record) || !record.valid ||
           record.revision!=bankRevision || record.config!=live_.sessionConfig() ||
           !BankS6::validatePatch(owner,record.values))return false;
        auto next=live_;
        for(int i=0;i<40;++i)
            if(!next.commitDesired(40*owner+i,record.values[static_cast<std::size_t>(i)],
                                   next.revision(),record.config))return false;
        live_=next;return true;
    }
    // S8 SAMPLE bank is the authority for 28 instrument PATCH parameters.
    // The 12 SPOT modes live in S8 configuration and are never touched here.
    // The S3 coordinator remains the ONLY writer of the shared LIVE image.
    bool recallNativeS8SamplePatch(int owner,const std::array<int,28>& values,
                                   int config,std::uint32_t expectedLiveRevision) noexcept {
        if(!diagnosticLive_||owner<0||owner>=8||config<1||config>127||
           live_.sessionConfig()!=config||live_.revision()!=expectedLiveRevision||
           longTx_.pending()||touch_.busy()||nav_.hasStage()||touchOwner_!=-1||
           expectedLiveRevision>LiveState::MaxRevision-28)return false;
        auto next=live_;
        for(int field=0;field<28;++field){
            const int v=values[static_cast<std::size_t>(field)];
            if(v<0||v>127||(field>=16&&v>1))return false;
            if(!next.commitDesired(40*owner+field,v,next.revision(),config))return false;
        }
        live_=next;return true;
    }
    // Native S8 MACHINE (source selected machine 1..24), no modes or BASE.
    // Stage thirty LIVE changes as one transaction; no partly applied patch.
    bool recallNativeS8MachinePatch(int machine,const std::array<int,30>& values,
                                   int config,std::uint32_t expectedLiveRevision) noexcept {
        if(!diagnosticLive_||machine<0||machine>=24||config<1||config>127||
           live_.sessionConfig()!=config||live_.revision()!=expectedLiveRevision||
           longTx_.pending()||touch_.busy()||nav_.hasStage()||touchOwner_!=-1||
           expectedLiveRevision>LiveState::MaxRevision-30)return false;
        auto next=live_;
        for(int field=0;field<30;++field){
            const int v=values[static_cast<std::size_t>(field)];
            if(v<0||v>127||(field>=18&&v>1))return false;
            if(!next.commitDesired(320+42*machine+field,v,next.revision(),config))return false;
        }
        live_=next;return true;
    }
    // Numeric RESET changes LIVE only. A stable LONG context keeps its NAV
    // return archive; unlike a PATCH recall, it must remain playable while held.
    bool commitNativeReset(const std::array<int,97>& cells,
            const std::array<int,97>& values,unsigned count,int config) noexcept {
        if(!diagnosticLive_||count==0||count>cells.size()||config!=live_.sessionConfig()||
           !nav_.stable()||touch_.busy()||nav_.hasStage()||touchOwner_!=-1)return false;
        auto next=live_;
        for(unsigned i=0;i<count;++i)
            if(!next.commitDesired(cells[i],values[i],next.revision(),config))return false;
        live_=next;return true;
    }
    // Qualified local source recalls use the same S3 LIVE writer. No backend ACK.
    bool commitNativeSourceRecall(const std::array<int,97>& cells,
            const std::array<int,97>& values,unsigned count,int config) noexcept {
        if(!diagnosticLive_||count==0||count>cells.size()||config!=live_.sessionConfig()||
           longTx_.pending()||touch_.busy()||nav_.hasStage()||touchOwner_!=-1)return false;
        auto next=live_;
        for(unsigned i=0;i<count;++i)
            if(!next.commitDesired(cells[i],values[i],next.revision(),config))return false;
        live_=next;return true;
    }
    // At most four source BASE97 images at one authenticated boundary.
    // Preserve unrelated observations and NAV; this is not a session reload.
    bool commitNativeAlternativeBases(const std::array<int,388>& cells,const std::array<int,388>& values,
                                     unsigned count,int config) noexcept {
        if(!diagnosticLive_||count==0||count>cells.size()||config!=live_.sessionConfig()||nav_.hasStage())return false;
        auto next=live_;
        for(unsigned i=0;i<count;++i)if(!next.commitDesired(cells[i],values[i],next.revision(),config))return false;
        live_=next;return true;
    }
    // Global recall has its own musical scope, not a lifecycle/session reload.
    // Keep S2/S3 held owners and the immutable LONG return archive alive (Q11).
    bool commitNativeGlobalRecall(const LiveState::SessionImage& image,const NavImage& navigation) noexcept {
        if(!diagnosticLive_||image.config!=live_.sessionConfig()||image.revision!=live_.revision()+1||
           !NavS3::validate(navigation)||nav_.hasStage())return false;
        auto nextLive=live_;auto nextNav=nav_;auto nextS3=s3_;
        if(!nextLive.restoreSession(image))return false;
        if(!(navigation==nextNav.active())&&(!nextNav.stage(navigation)||!nextNav.commit()||!nextS3.observeNav(navigation.at(292),true)))return false;
        if(!(navigation==nav_.active())){
            updateMachineFxNavigation(nav_,nextNav,longTx_.pending(),longTx_.pending(),machineFxBorrowed_,machineFxViewMachine_);machineFxHold_={};
        }
        live_=nextLive;nav_=nextNav;s3_=nextS3;return true;
    }
    bool commitNativeDocument(LiveState::SessionImage image,const NavImage& nav,const BankS6& bank,
            const std::array<std::uint32_t,96>& memoryBases,const std::array<std::uint32_t,84>& patchBases) noexcept {
        if(!diagnosticLive_||live_.revision()>=LiveState::MaxRevision||!NavS3::validate(nav)||nav_.hasStage())return false;
        image.revision=live_.revision()+1;
        auto nextLive=live_;auto nextNav=nav_;auto nextS3=s3_;
        if(!nextLive.restoreSession(image))return false;
        if(!(nav==nextNav.active())&&(!nextNav.stage(nav)||!nextNav.commit()||!nextS3.observeNav(nav.at(292),true)))return false;
        machineFxHold_={};machineFxBorrowed_=false;machineFxViewMachine_=nav.at(314);
        live_=nextLive;nav_=nextNav;s3_=nextS3;nativeBank_=bank;memoryRevisionBase_=memoryBases;patchRevisionBase_=patchBases;fx_=FxS6{};return true;
    }
    // Preserve document revisions without changing the frozen BankS6 corpus.
    bool readNativeMemory(int owner,int slot,BankS6::MemoryRecord& out)const noexcept {
        const int index=BankS6::memoryIndex(owner,slot);
        if(index<0||index>=96||!nativeBank_.readMemory(owner,slot,out))return false;
        out.revision+=memoryRevisionBase_[static_cast<unsigned>(index)];return true;
    }
    bool readNativePatch(int owner,int slot,BankS6::PatchRecord& out)const noexcept {
        const int index=BankS6::patchIndex(owner,slot);
        if(index<0||index>=84||!nativeBank_.readPatch(owner,slot,out))return false;
        out.revision+=patchRevisionBase_[static_cast<unsigned>(index)];return true;
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
        nav_=copy;s3_=s;machineFxHold_={};machineFxBorrowed_=false;machineFxViewMachine_=copy.active().at(314);return true;
    }
    int localSaveCandidateMode()const noexcept {return localSave_.bot<0?-1:localSave_.mode;}
    bool localSaveCandidateConsumed()const noexcept {return localSave_.consumed;}
    // Even a refused SAVE consumes the TOP/BOT ownership; no fallback
    // RECALL or stale solo NAV is permitted after a failed preflight.
    void declineLocalSaveCandidate() noexcept {
        if(localSave_.bot>=0)localSave_.consumed=true;
    }
    bool consumeLocalSaveCandidate(int expectedMode) noexcept {
        const int bot=localSave_.bot;
        if(bot<0||localSave_.consumed||localSave_.mode!=expectedMode||
           !surface_.held(static_cast<std::size_t>(bot))||
           s3_.physicalOwner(bot)!=20||
           !nav_.stable()||nav_.hasStage()||nav_.revision()!=localSave_.revision||
           nav_.active().at(292)!=expectedMode||longTx_.pending()||touch_.busy())return false;
        localSave_.consumed=true;return true;
    }
    const NavS3& nativeNav() const noexcept{return nav_;}
    NavRoute nativeRoute(int physical) const noexcept {
        const auto route=MachineFxAliases::route(nav_.active(),physical,
            machineFxBorrowed_&&nav_.active().at(314)==machineFxViewMachine_);
        if(route.address<0||route.position<0||route.position>=6)return route;
        const int family=(route.address%1024)/32;
        if((family!=5&&family!=6)||route.address%32!=8)return route;
        // DRUM8 and FILL8 share the existing BUS3/TOTAL INIT cells.
        const int globalFamily=11+route.position/3;
        if(physical>=40&&physical<52)return {32*globalFamily,route.position%3};
        if(physical>=20&&physical<32)return {1024+32*globalFamily,route.position%3};
        return route;
    }
    bool machineFxBorrowed()const noexcept{return machineFxBorrowed_&&nav_.active().contextMode==7&&nav_.active().at(314)==machineFxViewMachine_;}
    bool advanceMachineFxView(int64_t ms)noexcept {
        if(machineFxHold_.physical<0||!surface_.held(static_cast<unsigned>(machineFxHold_.physical)))return false;
        return qualifyMachineFxHold(machineFxHold_,machineFxBorrowed_,ms,nav_,longTx_.pending());
    }
    bool onHostStop()noexcept {
        if(!projection_.onHostStop())return false;
        live_.onHostStop();
        surface_.onHostStop();gesture_.onHostStop();s3_.onHostStop();longTx_.onHostStop();touch_.onHostStop();touchOwner_=-1;socleBase_={};
        localSave_={};machineFxHold_={};
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
                             const S3ProposalBatch& proposals,int& machineLow,int& machineHigh,
                             bool physicalTouchBusy,int localSaveBot) noexcept {
        djLoopStopPush_=false;
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
                // DJ LOOP contract: enter GLOBAL outside 2/4; retain GLOBAL/LOOP within.
                if(qualifiedNavDiagnostic_ && !tx.pending() && touchOwner_==-1 && !physicalTouchBusy && !nav.hasStage() &&
                   p.navAtDown==nav.active().at(292)){
                    // A rotation in GLOBAL/LOOP retains its page. Else enter GLOBAL.
                    if(nav.active().at(292)==2 || nav.active().at(292)==4 || nav.applyShortMode(2))
                        s3.observeNav(nav.active().at(292),true);
                }
                continue;
            }
            if(!normal&&!combo)continue;
            // A local PATCH candidate cannot open a solo NAV or LONG while
            // waiting for TOP (including 800 ms holds). Other BOT combos
            // retain full priority and are processed normally.
            if(normal && p.role==localSaveBot)continue;
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
            // Solo LONG qualifies at 200ms too; it must not open a temporary
            // NAV transaction while the same DJ PUSH is stopping an active loop.
            if(qualifiedNavDiagnostic_ && p.role==2 && retainDjLoopPage_ &&
               (nav.active().at(292)==2 || nav.active().at(292)==4) &&
               (p.qualifiedPhase==QualifiedPhase::Short || p.qualifiedPhase==QualifiedPhase::LongBegin)){
                djLoopStopPush_=true;continue;
            }
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
    struct MachineFxHold {int physical{-1};int64_t down{};uint64_t revision{};bool fired{};};
    static bool machineFxEligible(const NavS3& nav)noexcept {
        const auto& n=nav.active();const int selected=n.at(314);
        return nav.stable()&&!nav.hasStage()&&n.at(292)==7&&n.contextMode==7&&n.presetFlag==0&&selected>=2&&selected<=8&&selected%2==0;
    }
    static bool qualifyMachineFxHold(MachineFxHold& hold,bool& borrowed,int64_t ms,const NavS3& nav,bool longPending)noexcept {
        if(hold.physical<0||hold.fired||ms<hold.down||ms-hold.down<200||longPending||!machineFxEligible(nav)||nav.revision()!=hold.revision)return false;
        borrowed=!borrowed;hold.fired=true;return true;
    }
    // Presentation owner survives the existing LONG NAV archive. It is never
    // a second FX value/memory; other temporary MACHINE targets see native.
    static void updateMachineFxNavigation(const NavS3& before,const NavS3& after,bool wasLong,bool isLong,bool& borrowed,int& owner)noexcept {
        if(isLong)return;
        const auto& old=before.active();const auto& next=after.active();
        if((wasLong&&next.contextMode==7&&next.at(314)==owner)||
           (!wasLong&&old.contextMode==7&&next.contextMode==7&&old.at(314)==next.at(314)))return;
        borrowed=false;owner=next.contextMode==7?next.at(314):-1;
    }
    MachineFxHold machineFxHold_{};bool machineFxBorrowed_{};int machineFxViewMachine_{-1};
    SurfaceInput surface_{};
    struct LocalSaveCandidate {
        int bot{-1},mode{-1};std::uint64_t revision{};bool consumed{};
    };
    LocalSaveCandidate localSave_{};
    GestureS2 gesture_{};
    S3DecisionBoundary s3_{};
    bool retainDjLoopPage_{},djLoopStopPush_{};
    NavS3 nav_{};bool shortNavDiagnostic_{};bool qualifiedNavDiagnostic_{};bool muteBot2Exclusive_{};
    struct SocleBaseHold {int physical{-1};std::int64_t down{};NavImage before{};};
    SocleBaseHold socleBase_{};
    S3LongTransaction longTx_{};S3TouchNav touch_{};bool diagnosticTouch_{};int touchOwner_{-1};
    int machineLastLow_{-1},machineLastHigh_{-1};
    std::array<std::uint32_t,96> memoryRevisionBase_{};std::array<std::uint32_t,84> patchRevisionBase_{};
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
