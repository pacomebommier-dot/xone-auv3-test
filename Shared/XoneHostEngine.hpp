#pragma once
// Native V3: K2/gestures/NAV, known-session CC/LOOP and SMART/MIX/TOUCH/FLASH; specialized S8 remains pending.
#include "../CoreV3/CoreCoordinatorV3.hpp"
#include "XoneContinuousInput.hpp"
#include "XoneSoftMidiBridge.hpp"
#include "XoneTopPulseBus.hpp"
#include "XoneS8SamplePatchBank.hpp"
#include "XoneS8MachinePatchBank.hpp"
#include "XoneCueLoadBar.hpp"
#include "XoneSourcePatchBank.hpp"
#include "XoneHostScheduler.hpp"
#include "XoneGlobalSnapshot.hpp"
#include "XoneAlternativeArbiter.hpp"
#include "XoneAlternativeBank.hpp"
#include "XoneGroupActionGate.hpp"
#include "XoneSession.hpp"
#include "XoneDocument.hpp"
#include <memory>
#include "../Core/MutePhysicalGate.hpp"
#include "XoneMuteMidiProjection.hpp"
#include "XoneMuteLedOutput.hpp"
#include "XoneDjRelative.hpp"
#include "XoneDjBrowserWindow.hpp"
#include <atomic>
#include <array>
#include <cstdint>
#include <cstddef>
namespace xone::host {
enum class Stage : uint8_t { RxIn, Decision, TxOut, RxFeedback, Stop, Overflow, Gesture };
enum class Reason : uint8_t { Mapped, SurfaceOnly, Rejected, Untrusted, Sent, SendFailed, NoOutput, Received, Integrated, Qualified, ClockRejected, UnknownLive, PickupWait, Musical, DjRelative, BrowserLoadIntent, TopResetDeferred };
struct Trace {
    uint32_t sequence{};
    int64_t sample{};
    Stage stage{};
    Reason reason{};
    uint8_t cable{},length{},bytes[3]{};
    int32_t result{};
};
struct PendingPacket {uint8_t bytes[3]{};int api{-1},value{},type{10},key{127};bool requiresOff{};};
struct Result {
    bool emit{};
    uint8_t bytes[3]{};
    uint32_t sequence{};
    int64_t sample{};
    bool musical{};
    int api{-1},value{},type{10};
    std::array<PendingPacket,31> extra{}; // SAMPLE 28 / MACHINE 30 instrument packets
    unsigned extraCount{};
    int key{127};bool requiresOff{};int64_t gestureMs{};bool dj2Qualified{};
    unsigned packetCount() const noexcept {return emit?1+extraCount:0;}
    Result packet(unsigned i) const noexcept {
        if(!emit||i>extraCount)return {};
        Result r{};
        if(i==0){r={true,{bytes[0],bytes[1],bytes[2]},sequence,sample,musical,api,value,type};r.key=key;r.requiresOff=requiresOff;r.gestureMs=gestureMs;r.dj2Qualified=dj2Qualified;}
        else{const auto& p=extra[i-1];r={true,{p.bytes[0],p.bytes[1],p.bytes[2]},sequence,sample,true,p.api,p.value,p.type};r.key=p.key;r.requiresOff=p.requiresOff;}
        return r;
    }
};
class HostEngine {
public:
    HostEngine() noexcept {
        core_.initializeSourceDefaultNav();
        core_.enableDiagnosticLive(true);
        // Native logical configuration, independent of backend/instrument profile.
        // New LIVE stays entirely UNKNOWN; banks stay empty and no MIDI is sent.
        l01::LiveState::SessionImage local{};
        local.schema=1;local.config=1;local.revision=1;local.desired.fill(-2);
        core_.restoreNativeSessionForDiagnostic(local);
        core_.enableTestMidiProjection(true);
        core_.enableDiagnosticTouchNav(true); // source @zk/@zl selection, not SPOT TOUCH output
        core_.enableDiagnosticQualifiedNav(true); // source-qualified SHORT/LONG NAV only
        documents_->ui.image=captureDocument();
        // Opt-in native BOT2 overlay; legacy diagnostic harness retains NAV role1.
    }
    // Historical CH1 remap is an opt-in test seam, never production MIDI API.
    bool enableHistoricalL02BenchForDiagnostic(bool yes) noexcept {
        if(started_)return false;
        historicalL02Bench_=yes;return true;
    }
    // Pre-render test seam: exercise source-validated S3 routes without
    // installing a fictional instrument or mutating the historical NAV header.
    bool setNavForDiagnostic(const l01::NavImage& image) noexcept {
        return !started_&&core_.stageNativeNav(image)&&core_.commitNativeNav();
    }
    void enableNativeMutePage(bool yes) noexcept {mutePageEnabled_=yes;core_.enableNativeMuteBot2(yes);}
    bool mutePageVisible() const noexcept {return mutePageEnabled_&&muteGate_.visible();}
    bool muteGesturePending() const noexcept {return muteBridge_.state().pendingCount()!=0;}
    // Until a backend validates the deck, DJ1 remains MIDI-only. This
    // explicit diagnostic seam cannot authenticate any host/AUM destination.
    bool configureDjBrowserDeckForDiagnostic(int deck) noexcept {
        if(started_)return false;
        browser_.configureDeck(deck);
        core_.observeBrowserGuardForDiagnostic(false,0,0);
        return browser_.deck()!=0;
    }
    bool djBrowserPendingAt(int64_t ms) const noexcept {return browser_.pendingAt(ms);}
    bool djBrowserOwnsBot2() const noexcept {return browser_.pushOwned();}
    uint32_t browserLoadIntentsForDiagnostic() const noexcept {return browserLoadIntents_;}
    // Qualified DJ2 performance gesture is a request only: no audio engine
    // or synthetic LOOPY control is installed by this diagnostic seam.
    uint32_t dj2QualifiedTurnsForDiagnostic() const noexcept {return dj2QualifiedTurns_;}
    int dj2LastQualifiedDeltaForDiagnostic() const noexcept {return dj2LastQualifiedDelta_;}
    bool dj2PushRotationCapturedForDiagnostic() const noexcept {return dj2PushRotCaptured_;}
    // Requested state only; no backend status or audio-loop activation asserted.
    bool dj2LoopStartRequestedForDiagnostic() const noexcept {return dj2LoopStartRequested_;}
    uint32_t dj2LoopRequestsForDiagnostic() const noexcept {return dj2LoopRequests_;}
    uint32_t dj2LoopStopRequestsForDiagnostic() const noexcept {return dj2LoopStopRequests_;}
    // A backend adapter must capture a generation before asynchronous work.
    // In particular a late ON after transport STOP is stale, not authoritative.
    uint32_t dj2BackendGenerationForDiagnostic() const noexcept {return dj2BackendGeneration_;}
    // A requested activation is not proof of audio playback. A verified
    // backend can later report ON/OFF through this explicitly gated seam.
    enum class Dj2LoopObserved : uint8_t { Unknown, Off, On };
    Dj2LoopObserved dj2LoopObservedForDiagnostic() const noexcept {return dj2LoopObserved_;}
    bool observeDj2LoopBackendForDiagnostic(Dj2LoopObserved observed) noexcept {
        return observeDj2LoopBackendForDiagnostic(observed,dj2BackendGeneration_);
    }
    bool observeDj2LoopBackendForDiagnostic(Dj2LoopObserved observed,uint32_t generation) noexcept {
        if(observed==Dj2LoopObserved::Unknown||generation!=dj2BackendGeneration_)return false;
        dj2LoopObserved_=observed;
        if(observed==Dj2LoopObserved::Off){
            dj2LoopStartRequested_=false;
            if(djLoopCommand_==DjLoopCommand::Start)djLoopCommand_=DjLoopCommand::None;
        }
        return true;
    }
    // Explicit unresolved audio service boundary, separate from LOOP1/2/3.
    // Handoff acknowledges receipt only; playback still requires observation.
    enum class DjLoopCommand : uint8_t { None, Start, Stop, StopReset };
    struct DjLoopService {DjLoopCommand command{};uint32_t generation{};};
    DjLoopService pendingDjLoopService() const noexcept {return {djLoopCommand_,dj2BackendGeneration_};}
    bool acknowledgeDjLoopService(DjLoopService service) noexcept {
        if(service.command==DjLoopCommand::None || service.command!=djLoopCommand_ ||
           service.generation!=dj2BackendGeneration_)return false;
        djLoopCommand_=DjLoopCommand::None;return true;
    }
    bool s8SamplePatchForDiagnostic(int voice,int slot,S8SamplePatchBank::Patch& out)const noexcept {
        return voice>=0&&slot>=0&&s8Sample_.read(static_cast<unsigned>(voice),
                                                  static_cast<unsigned>(slot),out);
    }
    bool setS8SampleModeBeforeRendering(int voice,int index,int mode) noexcept { documentDirty_=true;
        return !started_&&voice>=0&&index>=0&&
            s8Sample_.setMode(static_cast<unsigned>(voice),static_cast<unsigned>(index),mode);
    }
    int s8SampleMode(int voice,int index)const noexcept {
        return voice>=0&&index>=0?s8Sample_.mode(static_cast<unsigned>(voice),
                                                static_cast<unsigned>(index)):-2;
    }
    bool samplePatchForDiagnostic(int owner,int slot,l01::BankS6::PatchRecord& out)const noexcept {
        return core_.readNativePatch(owner,slot,out);
    }
    bool s8MachinePatchForDiagnostic(int machine,int slot,S8MachinePatchBank::Patch& out)const noexcept {
        return machine>=0&&slot>=0&&s8Machine_.read(static_cast<unsigned>(machine),
                                                    static_cast<unsigned>(slot),out);
    }
    bool setS8MachineModeBeforeRendering(int machine,int index,int mode) noexcept { documentDirty_=true;
        return !started_&&machine>=0&&index>=0&&
            s8Machine_.setMode(static_cast<unsigned>(machine),static_cast<unsigned>(index),mode);
    }
    int s8MachineMode(int machine,int index)const noexcept {
        if(machine>=0&&machine<8&&index>=0&&index<12){const int instrument=activeAlternative(static_cast<unsigned>(machine/2));
            if(instrument>=0)return alternativeBank_.mode(static_cast<unsigned>(machine/4),static_cast<unsigned>(instrument),static_cast<unsigned>(12*(machine%2)+index));}
        return machine>=0&&index>=0?s8Machine_.mode(static_cast<unsigned>(machine),
                                                   static_cast<unsigned>(index)):-2;
    }
    unsigned sampleSavesAccepted()const noexcept {return sampleSavesAccepted_;}
    unsigned sampleSavesRefused()const noexcept {return sampleSavesRefused_;}
    unsigned topResetsDeferred() const noexcept {return topResetsDeferred_;}
    // Unified S7 ACTION FIFO. The S2/S3 caller MUST provide a qualified
    // captured owner; an arbitrary held button is not a musical GROUP.
    bool acceptQualifiedGroupAction(const QualifiedGroupAction& action) noexcept {
        if(action.physical>=groupDownEpoch_.size() ||
           action.physicalDownEpoch==0 ||
           groupDownEpoch_[action.physical]!=action.physicalDownEpoch ||
           !GroupActionGate::authorize(core_.nativeNav(),action,
               core_.surface().held(action.physical),core_.longNavPending()))
            return false;
        return topPulse_.queueGroup(action.fill,action.page,action.action);
    }
    std::uint64_t nativeNavRevisionForQualifiedSource() const noexcept {return core_.nativeNav().revision();}
    std::uint32_t physicalDownEpochForQualifiedSource(unsigned physical) const noexcept {
        return physical<groupDownEpoch_.size()?groupDownEpoch_[physical]:0;
    }
    unsigned pendingActionPulses() const noexcept {return topPulse_.pending();}
    unsigned pendingTopPulses() const noexcept {return topPulse_.pending();}
    TopPulsePacket nextActionPulse() const noexcept {return topPulse_.next();}
    TopPulsePacket nextTopPulse() const noexcept {return topPulse_.next();}
    void ackTopPulse(TopPulsePacket packet,bool success,int64_t sample) noexcept {
        if(topPulse_.acknowledge(packet,success))
            record(++sequence_,sample,Stage::TxOut,success?Reason::Sent:Reason::SendFailed,0,packet.bytes,3);
    }
    unsigned topPatchRecallsDeferred() const noexcept {return topPatchRecallsDeferred_;}
    enum class TopAction : uint8_t { FilterReset, PatchRecall };
    struct TopIntent {int physical{-1};l01::NavRoute route{};uint32_t sequence{};TopAction action{TopAction::FilterReset};};
    struct TopCapture {l01::NavRoute route{};int context{-2};std::uint64_t navRevision{};int root{-1};};
    bool lastTopIntent(unsigned index,TopIntent& out) const noexcept {
        if(index>=4||!topIntentValid_[index])return false;
        out=topIntents_[index];return true;
    }
    // One-shot transfer to the later PATCH/RESET resolver. Old commands
    // cannot be consumed after a newer physical gesture replaced the slot.
    // No LIVE mutation or MIDI output occurs during this handoff.
    bool consumeTopIntent(unsigned index,uint32_t expectedSequence,TopIntent& out) noexcept {
        if(index>=4||!topIntentValid_[index]||expectedSequence==0||
           topIntents_[index].sequence!=expectedSequence)return false;
        out=topIntents_[index];
        topIntentValid_[index]=false;
        return true;
    }
    // A selection command is not proof of file selection or completed LOAD.
    struct BrowserLoadRequest {int deck{};uint32_t selectionSequence{},sequence{};};
    bool pendingBrowserLoad(BrowserLoadRequest& request) const noexcept {
        if(!browserLoadPending_)return false;
        request=browserLoadRequest_;return true;
    }
    bool consumeBrowserLoad(uint32_t expectedSequence,BrowserLoadRequest& request) noexcept {
        if(!browserLoadPending_ || expectedSequence==0 || expectedSequence!=browserLoadRequest_.sequence ||
           browserLoadRequest_.selectionSequence!=browserSelectionSequence_)return false;
        request=browserLoadRequest_;browserLoadPending_=false;return true;
    }
    int lastBrowserLoadDeckForDiagnostic() const noexcept {return lastBrowserLoadDeck_;}
    // 16 neutral CUE/LOAD virtual CC. These report successful MIDI callback
    // delivery, NEVER a loaded synth preset or an applied XONE PATCH.
    static bool deriveMemory(const l01::NavS3& nav,int physical,int& owner,int& slot) noexcept {
        owner=slot=-1;
        if(!nav.stable()||nav.hasStage()||physical<4||physical>=20)return false;
        const auto& n=nav.active();if(n.contextMode!=0||n.presetFlag!=0||n.at(292)!=0)return false;
        auto route=n.route(physical);if(route.address<0)return false;
        const int kind=route.address/1024,family=(route.address%1024)/32,page=route.address%32;
        if(kind==5&&(family==3||family==4)&&page>=1&&page<=8&&page!=4&&page!=8){
            owner=6*(family-3)+page-1-(page>4?1:0);
        }else if(kind==8&&(family==5||family==6)&&page>=1&&page<=8){owner=12+8*(family-5)+page-1;}
        else return false;
        slot=route.position;return l01::BankS6::memoryIndex(owner,slot)>=0;
    }
    enum class SourceRecallStatus : uint8_t { None, Pending, AppliedLocal, EmptyPatch, UnknownConfig, Busy };
    int activeAlternative(unsigned source)const noexcept {return source<4?alternatives_.active()[source]:-1;}
    int pendingAlternative(unsigned source)const noexcept {return source<4?alternatives_.pending()[source]:-1;}
    int presetInstrumentGroup()const noexcept {return presetInstrumentGroup_;}
    bool consultPresetInstrumentGroup(unsigned source) noexcept {
        if(source>=4)return false;
        presetInstrumentGroup_=lastInstrumentGroup_=static_cast<int>(source);return true;
    }
    bool configureAlternativeBase(unsigned pool,unsigned instrument,const SourcePatchBank::Values& values,int config) noexcept { documentDirty_=true;
        return alternativeBank_.configureBase(pool,instrument,config,values);
    }
    bool bindAlternativeSlot(unsigned source,unsigned slot,int instrument) noexcept { documentDirty_=true;return alternativeBank_.bind(source,slot,instrument);}
    bool configureAlternativeMode(unsigned pool,unsigned instrument,unsigned index,int mode) noexcept { documentDirty_=true;return alternativeBank_.setMode(pool,instrument,index,mode);}
    AlternativeArbiter::Request requestAlternative(unsigned source,unsigned slot,std::uint64_t arbitration=0) noexcept {
        using Request=AlternativeArbiter::Request;
        if(source>=4||slot>=4||alternativeFlight_.valid)return Request::Rejected;
        const int instrument=alternativeBank_.binding(source,slot);SourcePatchBank::Values values{};
        if(instrument<0||!alternativeBank_.base(source/2,static_cast<unsigned>(instrument),core_.nativeLive().sessionConfig(),values))return Request::Rejected;
        if(arbitration==0){if(alternativeArbitration_==UINT64_MAX)return Request::Rejected;arbitration=alternativeArbitration_+1;}
        if(arbitration<alternativeArbitration_)return Request::Rejected;
        auto draft=alternatives_;const auto result=draft.request(source,instrument,arbitration);
        if(result==Request::Accepted&&!transportStopped_&&!musicalScheduler_.replace(5,0,ScheduleWhen::NextBar))return Request::Rejected;
        if(result==Request::Rejected)return result;
        alternatives_=draft;alternativeArbitration_=arbitration;
        if(result==Request::Accepted){alternativeBases_[source]=values;alternativeBaseValid_[source]=true;alternativePendingOrder_[source]=transportStopped_?0:musicalScheduler_.orderOf(5,0);}
        if(result==Request::Cancelled)alternativeBaseValid_[source]=false;
        if(transportStopped_&&result==Request::Accepted)beginAlternativeChange();
        return result;
    }
    void requestPresetPage(bool show) noexcept {presetFaceRequest_.store(show?1:0,std::memory_order_release);}
    bool presetPageVisibleForHost()const noexcept {return publishedPresetFace_.load(std::memory_order_acquire);}
    bool showPresetPage() noexcept { documentDirty_=true;
        if(core_.nativeNav().active().at(292)==1&&core_.nativeNav().active().presetFlag==1&&!mutePageVisible())return true;
        const auto before=presetSignature();auto n=core_.nativeNav().active();n.at(292)=1;n.presetFlag=1;
        if(!l01::NavS3::regenerate(n)||!core_.stageNativeNav(n)||!core_.commitNativeNav())return false;
        muteGate_.restoreVisible(false);presetInstrumentGroup_=lastInstrumentGroup_;invalidatePresetsIfChanged(before);return true;
    }
    bool leavePresetPage() noexcept { documentDirty_=true;
        if(core_.nativeNav().active().at(292)!=1||core_.nativeNav().active().presetFlag!=1)return true;
        const auto before=presetSignature();auto n=core_.nativeNav().active();n.at(292)=n.contextMode;n.presetFlag=0;
        if(!l01::NavS3::regenerate(n)||!core_.stageNativeNav(n)||!core_.commitNativeNav())return false;
        invalidatePresetsIfChanged(before);return true;
    }
    // INIT uses a separately supplied local reference, never a fabricated RESET zero.
    bool configureGlobalInit(const GlobalSnapshot& reference) noexcept { documentDirty_=true;
        if(initPending_||!reference.admissible(core_.nativeLive().sessionConfig()))return false;
        globalInit_=reference;return true;
    }
    bool requestGlobalInit() noexcept { documentDirty_=true;
        if(!globalInit_.admissible(core_.nativeLive().sessionConfig()))return false;
        if(transportStopped_)return applyGlobal(globalInit_,false);
        if(!musicalScheduler_.replace(4,0,ScheduleWhen::NextBeat))return false;
        initPending_=true;return true;
    }
    bool presetEngaged(unsigned slot)const noexcept {return slot<4&&presetEngaged_[slot];}
    bool presetMusicallyValid(unsigned slot)const noexcept {return slot<4&&presetValid_[slot];}
    int pendingPreset()const noexcept {return pendingPresetSlot_>=0?pendingPresetSlot_:(alternativeFlight_.valid&&alternativeFlight_.global?alternativeFlight_.presetSlot:-1);}
    bool redActive()const noexcept {return redActive_;}
    bool alternativeSwitchPending()const noexcept {return alternativeFlight_.valid;}
    bool capturePreset(unsigned slot) noexcept { documentDirty_=true;
        if(slot>=4||redActive_)return false;
        const bool before=presetEngaged_[slot];presetEngaged_[slot]=true;
        const auto image=captureGlobal();
        if(!image.admissible(core_.nativeLive().sessionConfig())){presetEngaged_[slot]=before;return false;}
        presets_[slot]=image;presetValid_[slot]=false;return true;
    }
    bool requestPresetRecall(unsigned slot) noexcept { documentDirty_=true;
        if(slot>=4||!presets_[slot].admissible(core_.nativeLive().sessionConfig()))return false;
        if(transportStopped_){
            applyingPresetSlot_=static_cast<int>(slot);const bool applied=applyGlobal(presets_[slot],false);applyingPresetSlot_=-1;
            if(!applied)return false;
        }else{
            if(!musicalScheduler_.replace(3,0,ScheduleWhen::NextBar))return false;
            pendingPresetImage_=presets_[slot];pendingPresetSlot_=static_cast<int>(slot);
        }
        presetValid_[slot]=true;return true;
    }
    bool captureRed() noexcept { documentDirty_=true;
        if(redActive_)return false;
        const auto image=captureGlobal();
        if(!image.admissible(core_.nativeLive().sessionConfig()))return false;
        redImage_=image;redActive_=true;return true;
    }
    bool restoreRed() noexcept { documentDirty_=true;
        if(!redActive_||!applyGlobal(redImage_,true))return false;
        redActive_=false;redImage_.valid=false;return true;
    }
    bool readPreset(unsigned slot,GlobalSnapshot& out)const noexcept {
        if(slot>=4)return false;
        out=presets_[slot];return out.valid;
    }
    // Native commands use contractual owner IDs, not external destinations.
    bool captureMemory(int owner,int slot) noexcept { documentDirty_=true;
        const int index=l01::BankS6::memoryIndex(owner,slot);
        if(index<0||(owner<12&&alternativeSourceBusy(owner/3))||!core_.captureNativeMemory(owner,slot))return false;
        memoryEngaged_[static_cast<unsigned>(index)]=true;return true;
    }
    void observeTransport(bool running) noexcept {if(running&&transportStopped_)muteLeds_.invalidate();transportStopped_=!running;}
    void forgetTransport() noexcept {transportStopped_=false;}
    bool requestMemoryRecall(int owner,int slot) noexcept { documentDirty_=true;
        if(!memoryEngaged(owner,slot)||(owner<12&&alternativeSourceBusy(owner/3)))return false;
        l01::BankS6::MemoryRecord saved{};
        if(!core_.readNativeMemory(owner,slot,saved)||!saved.valid||saved.config!=core_.nativeLive().sessionConfig())return false;
        if(!transportStopped_&&!musicalScheduler_.replace(2,static_cast<uint16_t>(owner),ScheduleWhen::NextBeat))return false;
        memoryPending_[static_cast<unsigned>(owner)]={slot,saved.config,saved.values};
        if(transportStopped_)applyMemoryRecall(static_cast<unsigned>(owner));
        return true;
    }
    bool requestSourceRecall(unsigned source,unsigned choice) noexcept { documentDirty_=true;
        if(source>=4||choice>3||alternativeSourceBusy(static_cast<int>(source)))return false;
        if(!transportStopped_&&!musicalScheduler_.replace(1,static_cast<uint16_t>(source),ScheduleWhen::NextBar))return false;
        sourcePending_[source]=static_cast<int>(choice);sourcePendingAlternative_[source]=activeAlternative(source);sourceStatus_[source]=SourceRecallStatus::Pending;
        if(transportStopped_)applySourceRecall(source);
        return true;
    }
    bool memoryEngaged(int owner,int slot)const noexcept {
        int index=l01::BankS6::memoryIndex(owner,slot);return index>=0&&memoryEngaged_[static_cast<unsigned>(index)];
    }
    int pendingMemorySlot(unsigned owner)const noexcept {return owner<28?memoryPending_[owner].slot:-1;}
    bool readMemory(int owner,int slot,l01::BankS6::MemoryRecord& out)const noexcept {return core_.readNativeMemory(owner,slot,out);}
    // Render-thread local source API. An external adapter is never treated as loaded.
    bool saveSourcePatch(unsigned source,unsigned slot) noexcept { documentDirty_=true;
        if(source>=4||slot>=3||sourcePending_[source]>=0||redActive_)return false;
        SourcePatchBank::Values values{};
        for(unsigned f=0;f<values.size();++f){
            for(const auto& hold:spots_)if(hold.cell==SourcePatchBank::cell(source,f))return false;
            auto v=core_.nativeLive().desired(SourcePatchBank::cell(source,f));
            if(!v.known)return false;
            values[f]=v.value;
        }
        if(alternativeFlight_.valid&&alternativeFlight_.plan.from[source]!=alternativeFlight_.plan.to[source])return false;
        const int instrument=activeAlternative(source);
        if(instrument>=0)return alternativeBank_.save(source/2,static_cast<unsigned>(instrument),slot,core_.nativeLive().sessionConfig(),values);
        return sourcePatches_.save(source,slot,core_.nativeLive().sessionConfig(),values);
    }
    bool readSourcePatch(unsigned source,unsigned slot,SourcePatchBank::Patch& out)const noexcept {
        const int instrument=activeAlternative(source);
        return instrument>=0?alternativeBank_.patch(source/2,static_cast<unsigned>(instrument),slot,out):sourcePatches_.read(source,slot,out);
    }
    SourceRecallStatus sourceRecallStatus(unsigned source)const noexcept {
        return source<4?sourceStatus_[source]:SourceRecallStatus::None;
    }
    struct SourcePacket {bool valid{};unsigned token{};int type{},api{-1},value{};std::array<uint8_t,3> bytes{};int key{127};std::uint64_t closure{};};
    SourcePacket nextSourcePacket()const noexcept {
        if(sourceRead_==sourceWrite_||sourceBlocked_)return {};
        return sourceMidi_[sourceRead_%sourceMidi_.size()];
    }
    void beginSourceRetryPass() noexcept {sourceBlocked_=false;}
    void ackSourcePacket(const SourcePacket& packet,bool success,int64_t sample) noexcept {
        const auto current=nextSourcePacket();
        if(!current.valid||packet.token!=current.token)return;
        Result r{true,{current.bytes[0],current.bytes[1],current.bytes[2]},sequence_,sample,true,current.api,current.value,current.type};
        r.key=current.key;sent(r,success?0:-1);
        if(success){
            ++sourceRead_;
            if(current.closure!=0&&alternativeFlight_.valid&&current.closure==alternativeFlight_.serial&&alternativeFlight_.waiting){
                --alternativeFlight_.waiting;
                if(alternativeFlight_.waiting==0)finishAlternativeChange();
            }
        }else sourceBlocked_=true;
    }
    int cueLoadSent(unsigned group) const noexcept {return cueLoad_.sentSelection(group);}
    int cueLoadPending(unsigned group) const noexcept {return cueLoad_.pendingSelection(group);}
    unsigned pendingCueLoad() const noexcept {return cueLoad_.pendingActions();}
    CueLoadBar::Packet nextCueLoadPacket() const noexcept {return cueLoad_.next();}
    void beginCueLoadRetryPass() noexcept {cueLoad_.beginRetryPass();}
    void ackCueLoadPacket(CueLoadBar::Packet packet,bool ok,int64_t sample) noexcept {
        if(cueLoad_.acknowledge(packet,ok))
            record(++sequence_,sample,Stage::TxOut,ok?Reason::Sent:Reason::SendFailed,
                   0,packet.bytes.data(),3,packet.api);
    }
    const GlobalMuteState& globalMute() const noexcept {return muteBridge_.state();}
    unsigned onMusicalGrid(const GridBatch& b) noexcept { documentDirty_=true;
        if(b.valid&&!b.overflow&&b.count<=b.events.size())started_=true;
        unsigned committed=soft_.observe(b); // only authenticated BAR, never timer
        if(b.valid&&!b.overflow&&b.count<=b.events.size()){
            musicalScheduler_.consume(b,[this](DueAction due) noexcept -> bool {
                if(due.action.command==1){if(alternativeFlight_.valid&&alternativeFlight_.global)return false;applySourceRecall(due.action.destination);}
                else if(due.action.command==2){if(alternativeFlight_.valid&&alternativeFlight_.global)return false;applyMemoryRecall(due.action.destination);}
                else if(due.action.command==5)return beginAlternativeChange();
                else if(due.action.command==4){
                    if(!initPending_)return true;
                    applyingGlobalOrder_=due.action.order;const bool applied=applyGlobal(globalInit_,false);applyingGlobalOrder_=0;
                    if(!applied)return false;
                    initPending_=false;
                }else if(due.action.command==3){
                    const int slot=pendingPresetSlot_;
                    if(slot<0)return true;
                    applyingPresetSlot_=slot;applyingGlobalOrder_=due.action.order;
                    const bool applied=applyGlobal(pendingPresetImage_,false);applyingPresetSlot_=-1;applyingGlobalOrder_=0;
                    if(!applied)return false;
                    pendingPresetSlot_=-1;presetValid_[static_cast<unsigned>(slot)]=true;
                }
                return true;
            });
            for(unsigned i=0;i<b.count;++i)if(b.events[i].kind==GridKind::Bar)cueLoad_.onBar();
        }
        if(mutePageEnabled_){
            const unsigned muteCommitted=muteBridge_.observe(b);
            if(muteCommitted)muteMidi_.observe(muteBridge_.state());
            committed+=muteCommitted;
        }
        return committed;
    }
    SoftMidiPacket nextSoftPacket(unsigned& cursor) const noexcept {return soft_.next(cursor);}
    void ackSoftPacket(const SoftMidiPacket& packet,bool callbackSucceeded) noexcept {
        soft_.confirm(packet,callbackSucceeded);
    }
    int appliedSoftChoice(unsigned family,unsigned page) const noexcept {return soft_.active(family,page);}
    unsigned pendingSoftCount() const noexcept {return soft_.pendingCount();}
    MuteMidiProjection::Packet nextMutePacket(unsigned& cursor) const noexcept {
        return mutePageEnabled_?muteMidi_.next(cursor):MuteMidiProjection::Packet{};
    }
    void ackMutePacket(unsigned target,bool callbackSucceeded) noexcept {
        muteMidi_.confirm(target,callbackSucceeded);
    }
    MuteLedOutput::Image ledImage()const noexcept {
        MuteLedOutput::Image image{};const auto& nav=core_.nativeNav();if(!nav.stable()||nav.hasStage())return image;
        const auto& n=nav.active();const auto base=BaseLedView::compose(nav);
        for(unsigned i=0;i<16;++i)image[i]=base[i];
        const auto set=[&](unsigned i,MuteLedColor color,bool blink=false){image[i]={color,blink,true};};
        const int config=core_.nativeLive().sessionConfig();
        for(unsigned i=0;i<32;++i){const int physical=static_cast<int>(i)+4;const auto route=n.route(physical);
            if(route.address==-1&&route.position==-1){set(i,MuteLedColor::Off);continue;}
            if(route.address<0||route.position<0)continue;
            const int kind=route.address/1024,family=(route.address%1024)/32,page=route.address%32,slot=route.position;
            int cell=-1,modeCell=-1;
            if(kind==3&&l01::LiveState::spotAddresses(route.address,slot,cell,modeCell)){
                auto mode=modeCell>=0?core_.nativeLive().desired(modeCell):l01::LiveRead{true,1};
                if(cell<320&&cell%40>=16&&cell%40<28){const int m=s8SampleMode(cell/40,cell%40-16);if(m>=1&&m<=4)mode={true,m};}
                else if(cell>=320&&cell<1328&&(cell-320)%42>=18&&(cell-320)%42<30){
                    const int machine=(cell-320)/42,m=s8MachineMode(machine,(cell-320)%42-18);
                    if(m>=1&&m<=4)mode={true,m};else if(machine<8&&activeAlternative(static_cast<unsigned>(machine/2))>=0)mode={};
                }
                const auto value=core_.nativeLive().desired(cell);
                if(value.known&&value.value<=1&&mode.known&&mode.value>=1&&mode.value<=4){
                    MuteLedColor color=MuteLedColor::Red;
                    if(family>=9&&family<=12)color=static_cast<MuteLedColor>(1+(family-9)%3);
                    else if(mode.value==2||family==5||family==14)color=MuteLedColor::Green;
                    else if(mode.value==3||family==4)color=MuteLedColor::Yellow;
                    set(i,value.value?color:MuteLedColor::Off);
                }
            }else if(kind==9&&(family==7||family==8)&&page==0&&slot<6){
                const auto v=core_.nativeLive().desired(1744+12*(family-7)+slot);
                if(v.known)set(i,v.value==63?MuteLedColor::Off:static_cast<MuteLedColor>(family-6));
            }
            int memoryOwner=-1,memorySlot=-1;
            if(deriveMemory(nav,physical,memoryOwner,memorySlot)){
                set(i,memoryEngaged(memoryOwner,memorySlot)?MuteLedColor::Red:MuteLedColor::Off);
                if(pendingMemorySlot(static_cast<unsigned>(memoryOwner))==memorySlot)set(i,MuteLedColor::Green,true);
            }
            if(physical>=4&&physical<=16&&(physical-4)%4==0&&slot==(physical-4)/4){
                const int root=n.at(291);int owner=-1;
                if(root>=0&&root<=1){
                    if(family==3+root&&page>=5&&page<=8&&kind==(page==8?6+root:5))owner=4*root+page-5;
                    else if(kind==8&&family==5+root&&page>=1&&page<=8)owner=8+page-1;
                }
                if(owner>=0){const int active=soft_.active(static_cast<unsigned>(owner/8),static_cast<unsigned>(owner%8));
                    if(active>=0)set(i,active==slot?MuteLedColor::Green:MuteLedColor::Off);
                    if(soft_.pendingChoice(static_cast<unsigned>(owner))==slot)set(i,MuteLedColor::Green,true);
                }
            }
            unsigned cueGroup=0,cueIndex=0;
            if(CueLoadBar::derive(nav,static_cast<unsigned>(physical),cueGroup,cueIndex)){
                set(i,cueLoad_.sentSelection(cueGroup)==static_cast<int>(cueIndex)?MuteLedColor::Green:MuteLedColor::Off);
                if(cueLoad_.pendingSelection(cueGroup)==static_cast<int>(cueIndex)||cueLoad_.dueSelection(cueGroup)==static_cast<int>(cueIndex))set(i,MuteLedColor::Yellow,true);
            }
            if(kind==11&&slot<3){
                bool valid=false,current=false,known=true,compatible=true;int selected=-1;const bool machine=n.contextMode==7&&n.at(292)==7;
                if(machine)selected=n.at(314)-1;else if(family==1)selected=page-1;
                if(selected>=0&&selected<(machine?24:8)){
                    if(machine&&selected<8){SourcePatchBank::Patch patch{};
                        const bool exists=readSourcePatch(static_cast<unsigned>(selected/2),static_cast<unsigned>(slot),patch);compatible=!exists||patch.config==config;valid=exists&&compatible;current=valid;
                        if(valid)for(unsigned f=0;f<60;++f){const auto v=core_.nativeLive().desired(SourcePatchBank::cell(static_cast<unsigned>(selected/2),f));if(!v.known)known=false;else if(v.value!=patch.values[f])current=false;}
                    }else if(machine){S8MachinePatchBank::Patch patch{};valid=s8Machine_.read(static_cast<unsigned>(selected),static_cast<unsigned>(slot),patch)&&patch.valid&&patch.config==config;compatible=!patch.valid||patch.config==config;current=valid;
                        if(valid)for(unsigned f=0;f<30;++f){const auto v=core_.nativeLive().desired(320+42*selected+static_cast<int>(f));if(!v.known)known=false;else if(v.value!=patch.values[f])current=false;}
                    }else {S8SamplePatchBank::Patch patch{};valid=s8Sample_.read(static_cast<unsigned>(selected),static_cast<unsigned>(slot),patch)&&patch.valid&&patch.config==config;compatible=!patch.valid||patch.config==config;current=valid;
                        if(valid)for(unsigned f=0;f<28;++f){const auto v=core_.nativeLive().desired(40*selected+static_cast<int>(f));if(!v.known)known=false;else if(v.value!=patch.values[f])current=false;}
                        if(!valid&&compatible){l01::BankS6::PatchRecord legacy{};valid=core_.readNativePatch(selected,slot,legacy)&&legacy.valid&&legacy.config==config;compatible=!legacy.valid||legacy.config==config;current=valid;
                            if(valid)for(unsigned f=0;f<28;++f){const auto v=core_.nativeLive().desired(40*selected+static_cast<int>(f));if(!v.known)known=false;else if(v.value!=legacy.values[f])current=false;}}
                    }
                    if(!compatible||(valid&&current&&!known))image[i]={};else set(i,!valid?MuteLedColor::Green:current?MuteLedColor::Red:MuteLedColor::Yellow);
                }
            }
        }
        if(n.at(292)==1&&n.presetFlag==1&&!mutePageVisible()){
            for(unsigned slot=0;slot<4;++slot){set(slot,!presetEngaged(slot)?MuteLedColor::Green:presetMusicallyValid(slot)?MuteLedColor::Red:MuteLedColor::Yellow);
                if(pendingPreset()==static_cast<int>(slot))set(slot,MuteLedColor::Yellow,true);}
            set(4,MuteLedColor::Green,initPending_);
            for(unsigned i=5;i<8;++i)set(i,MuteLedColor::Off);
            for(unsigned group=0;group<4;++group)set(8+group,presetInstrumentGroup_==static_cast<int>(group)?MuteLedColor::Red:MuteLedColor::Off);
            for(unsigned slot=0;slot<4;++slot){image[12+slot]={};if(presetInstrumentGroup_<0)continue;
                const auto group=static_cast<unsigned>(presetInstrumentGroup_);const int id=alternativeBank_.binding(group,slot);
                if(id<0)continue;
                set(12+slot,MuteLedColor::Off);const unsigned peer=group^1u;
                if(activeAlternative(group)==id)set(12+slot,MuteLedColor::Green);
                else if(activeAlternative(peer)==id)set(12+slot,MuteLedColor::Yellow);
                else if(pendingAlternative(group)==id)set(12+slot,MuteLedColor::Green,true);
                else if(pendingAlternative(peer)==id)set(12+slot,MuteLedColor::Yellow);
            }
        }
        set(32,redActive_?MuteLedColor::Red:MuteLedColor::Off);
        set(33,(n.at(292)==7?n.at(315):n.at(291))==1?MuteLedColor::Yellow:MuteLedColor::Off);
        if(mutePageVisible()){const auto mute=MuteLedView::compose(globalMute());for(unsigned i=0;i<16;++i)set(i,mute[i].color,mute[i].blinking);}
        return image;
    }
    void pickupLedFeedback(int cc,int64_t ms) noexcept {
        if(cc<4||cc>15)return;
        const auto route=core_.nativeNav().active().route(16+cc);
        int cell=-1,modeCell=-1;
        if(route.address<0||route.address/1024!=3||!l01::LiveState::spotAddresses(route.address,route.position,cell,modeCell))return;
        const int family=(route.address%1024)/32;int mode=1;
        if(family==1||family==2){auto v=core_.nativeLive().desired(modeCell);mode=v.known?v.value:-2;
            if(family==1){const int m=s8SampleMode(cell/40,cell%40-16);if(m>=1&&m<=4)mode=m;}
            else {const int machine=(cell-320)/42,m=s8MachineMode(machine,(cell-320)%42-18);
                if(m>=1&&m<=4)mode=m;else if(machine<8&&activeAlternative(static_cast<unsigned>(machine/2))>=0)mode=-2;}
        }
        if(mode<1||mode>4)return;
        muteLeds_.flash(static_cast<unsigned>(cc+12),mode==2?MuteLedColor::Green:mode==3?MuteLedColor::Yellow:MuteLedColor::Red,ms);
    }
    void requestLedRefresh() noexcept {ledRefreshRequest_.store(true,std::memory_order_release);}
    void refreshLeds() noexcept {muteLeds_.invalidate();}
    void prepareMuteLeds(int64_t ms) noexcept {
        if(ledNavRevision_!=core_.nativeNav().revision()){muteLeds_.cancelFlashes();ledNavRevision_=core_.nativeNav().revision();}
        muteLeds_.observeFull(ledImage(),ms);
    }
    MuteLedOutput::Packet nextMuteLedPacket(unsigned& cursor) const noexcept {return muteLeds_.next(cursor);}
    void ackMuteLedPacket(const MuteLedOutput::Packet& packet,bool success) noexcept {muteLeds_.confirm(packet,success);}
    static constexpr uint32_t TraceCapacity=256;
    static_assert(std::atomic<uint32_t>::is_always_lock_free,"RT trace requires lock-free indices");
    // Called only by render thread. Host cable is NOT an authenticated device ID.
    Result receive(uint8_t cable,const uint8_t* data,size_t length,int64_t sample,int64_t gestureMs=0) noexcept { documentDirty_=true;
        const auto before=presetSignature();const auto applied=globalApplySerial_;redDirectGesture_=false;
        auto result=receiveImpl(cable,data,length,sample,gestureMs);
        if(cable==0&&data&&length==3&&(data[0]&15)==14&&((data[0]&0xf0)==0x80||(data[0]&0xf0)==0x90)&&
           !redDirectGesture_&&applied==globalApplySerial_)invalidatePresetsIfChanged(before);
        return result;
    }
private:
    Result receiveImpl(uint8_t cable,const uint8_t* data,size_t length,int64_t sample,int64_t gestureMs=0) noexcept {
        started_=true;
        const auto seq=++sequence_;
        record(seq,sample,cable?Stage::RxFeedback:Stage::RxIn,Reason::Received,cable,data,length);
        if(cable) {record(seq,sample,Stage::Decision,Reason::Untrusted,cable,data,length);return {};}
        if(!data || length!=3 || data[1]>127 || data[2]>127) {
            record(seq,sample,Stage::Decision,Reason::Rejected,cable,data,length);return {};
        }
        // Cable 0 is a route boundary, not authenticated hardware identity.
        if((data[0]&15)==14) {
            const int cmd=data[0]&0xF0;
            const bool valid=((cmd==0x90||cmd==0x80)&&l01::SurfaceInput::physicalControl(data[1])>=0)
                ||(cmd==0xB0&&data[1]<=21);
            if(!valid) {record(seq,sample,Stage::Decision,Reason::Rejected,cable,data,length);return {};}
            const int physical=l01::SurfaceInput::physicalControl(data[1]);
            const bool wasHeld=physical>=0&&core_.surface().held(physical);
            const auto softRoute=(physical>=4&&physical<20)?core_.nativeNav().active().route(physical):l01::NavRoute{};
            const int softRoot=core_.nativeNav().active().at(291);
            // BOT2 is also DJ1 PUSH. Ownership is captured on its true DOWN,
            // never reconstructed from a deadline at UP.
            const bool browserBot2Release=physical==1&&wasHeld&&browser_.pushOwned();
            const bool localSampleBot2Release=physical==1&&wasHeld&&
                !(cmd==0x90&&data[2]!=0)&&core_.localSaveCandidateMode()==3;
            const bool localSampleBot2Consumed=core_.localSaveCandidateConsumed();
            const bool wasMuteVisible=mutePageVisible();
            auto muteNext=muteGate_;muteNext.advance(gestureMs);
            const bool baseEvent=(cmd==0x90||cmd==0x80)&&physical>=4&&physical<20;
            const bool muteBaseOwned=mutePageEnabled_&&baseEvent&&
                (wasHeld?muteGate_.ownsBase(static_cast<unsigned>(physical-4)):muteNext.visible());
            // Latch the original CUE/LOAD identity BEFORE S2/S3 processes
            // this physical DOWN. The current NAV after processing may have
            // changed; it must never redirect the original source command.
            unsigned cueGroup=0,cueIndex=0;
            const bool cueSource=baseEvent && !muteBaseOwned && !wasHeld &&
                cmd==0x90&&data[2]!=0&&!core_.longNavPending()&&
                CueLoadBar::derive(core_.nativeNav(),static_cast<unsigned>(physical),
                                   cueGroup,cueIndex);
            int memoryOwner=-1,memorySlot=-1;
            const bool memorySource=baseEvent&&!muteBaseOwned&&!wasHeld&&cmd==0x90&&data[2]!=0&&
                !core_.longNavPending()&&deriveMemory(core_.nativeNav(),physical,memoryOwner,memorySlot);
            const bool redReleaseOwned=physical==0&&wasHeld&&core_.s3().physicalOwner(0)==0;
            const auto presetRoute=physical>=4&&physical<20?core_.nativeNav().active().route(physical):l01::NavRoute{};
            const bool presetSource=baseEvent&&!muteBaseOwned&&!wasHeld&&cmd==0x90&&data[2]!=0&&
                core_.nativeNav().active().presetFlag==1&&presetRoute.address==14*1024+17*32&&presetRoute.position>=0&&presetRoute.position<=4;
            const bool wasPresetFrame=core_.nativeNav().active().at(292)==1&&core_.nativeNav().active().presetFlag==1&&!mutePageVisible();
            l01::NavRoute inputRoute{};
            if(physical>=4)inputRoute=core_.nativeNav().active().route(physical);
            else if(cmd==0xB0&&data[1]<20)inputRoute=core_.nativeNav().active().route(36+data[1]);
            const int inputSource=sourceForRoute(inputRoute);
            const bool newSourceCommand=(cmd==0xB0&&data[1]<20)||(cmd==0x90&&data[2]!=0&&!wasHeld);
            if(newSourceCommand&&alternativeSourceBusy(inputSource)&&
               (inputSource>=0||(alternativeFlight_.global&&cmd==0xB0&&data[1]<20))){
                record(seq,sample,Stage::Decision,Reason::Rejected,0,data,3);return {};
            }
            const int previousNav=navMode();
            core_.retainDjLoopPage(dj2LoopObserved_!=Dj2LoopObserved::Off || dj2LoopStartRequested_);
            const bool accepted=core_.onMidi({14,cmd,data[1],data[2]},gestureMs,muteBaseOwned);
            record(seq,sample,Stage::Decision,accepted?Reason::Integrated:Reason::ClockRejected,cable,data,length);
            if(accepted)resolveDjLoopNavigation(previousNav);
            if(accepted&&inputSource>=0&&newSourceCommand)lastInstrumentGroup_=inputSource;
            if(accepted&&!wasPresetFrame&&core_.nativeNav().active().at(292)==1&&core_.nativeNav().active().presetFlag==1&&!mutePageVisible())
                presetInstrumentGroup_=lastInstrumentGroup_;
            if(accepted&&physical>=12&&physical<20){
                auto& selector=alternativeSelectors_[static_cast<unsigned>(physical-12)];
                const bool down=cmd==0x90&&data[2]!=0;
                if(down&&!wasHeld&&wasPresetFrame)selector={true,physical<16?physical-12:presetInstrumentGroup_,physical<16?-1:physical-16,gestureMs,core_.nativeNav().revision()};
                else if(!down&&wasHeld&&selector.held){
                    const auto captured=selector;selector={};
                    if(wasPresetFrame&&gestureMs>=captured.down&&gestureMs-captured.down<200&&captured.navRevision==core_.nativeNav().revision()){
                        if(captured.slot<0)consultPresetInstrumentGroup(static_cast<unsigned>(captured.source));
                        else if(captured.source>=0)requestAlternative(static_cast<unsigned>(captured.source),static_cast<unsigned>(captured.slot));
                    }
                }
            }
            // A GROUP source proof lives only between a TRUE DOWN and its
            // TRUE UP; STOP invalidates the epoch even if SurfaceInput keeps
            // the physical Held bit until the real release.
            if(accepted&&physical>=0&&physical<36&&(cmd==0x90||cmd==0x80)){
                const bool down=cmd==0x90&&data[2]!=0;
                if(down&&!wasHeld){
                    ++groupEpochCounter_;
                    if(groupEpochCounter_==0)++groupEpochCounter_;
                    groupDownEpoch_[static_cast<unsigned>(physical)]=groupEpochCounter_;
                    if(physical>=20&&physical<32){
                        auto& hold=groupHolds_[static_cast<unsigned>(physical-20)];
                        hold={};
                        QualifiedGroupAction action{};
                        if(GroupActionGate::derive(core_.nativeNav(),
                            static_cast<unsigned>(physical),
                            groupDownEpoch_[static_cast<unsigned>(physical)],action) &&
                            !core_.longNavPending() && !mutePageVisible()){
                            hold={action,gestureMs,true,false};
                        }
                    }
                }else if(!down&&wasHeld){
                    if(physical>=20&&physical<32){
                        auto& hold=groupHolds_[static_cast<unsigned>(physical-20)];
                        // Qualify the long physical release while its original
                        // DOWN still owns the key; not an orphan UP or new page.
                        fireGroupLong(hold,gestureMs,true);
                        hold={};
                    }
                    groupDownEpoch_[static_cast<unsigned>(physical)]=0;
                }
            }
            // Only real, non-duplicate physical DOWN can arm the 4 choices on
            // a source-qualified DRUMS/FILL page. No synthetic selection.
            if(accepted&&!wasHeld&&cmd==0x90&&data[2]!=0&&
               !muteBaseOwned&&physical>=4&&physical<20&&
               !core_.longNavPending()&&!core_.nativeNav().hasStage())
                soft_.requestFromRoute(physical,softRoute,softRoot);
            if(accepted && cueSource){
                cueLoad_.requestQualified(cueGroup,cueIndex);
                if(transportStopped_)cueLoad_.onStopped();
                requestSourceRecall(cueGroup,cueIndex%4);
            }
            if(accepted&&memorySource)memoryGesture(memoryOwner,memorySlot);
            if(accepted&&presetSource){
                const unsigned slot=static_cast<unsigned>(presetRoute.position);
                if(slot==4)requestGlobalInit();
                else if(!presetEngaged(slot))capturePreset(slot);
                else requestPresetRecall(slot);
            }
            if(accepted&&physical==0){
                const bool down=cmd==0x90&&data[2]!=0;
                if(down&&!wasHeld&&core_.s3().physicalOwner(0)==0){
                    redDirectGesture_=true;redWasActive_=redActive_;redDownMs_=gestureMs;
                    redHeld_=redActive_||captureRed();
                }else if(!down&&wasHeld&&redHeld_&&redReleaseOwned){
                    redDirectGesture_=true;redHeld_=false;
                    if(redWasActive_||gestureMs-redDownMs_>=200)restoreRed();
                }
            }
            drainCore(seq,sample);
            if(accepted&&physical==2&&(cmd==0x90||cmd==0x80)&&
               wasHeld&&!(cmd==0x90&&data[2]!=0))dj2PushRotCaptured_=false;
            if(accepted&&physical==1&&(cmd==0x90||cmd==0x80)){
                const bool down=cmd==0x90&&data[2]!=0;
                if(down&&!wasHeld&&core_.s3().physicalOwner(1)==23){
                    browser_.beginPush(gestureMs);
                }else if(!down&&browserBot2Release){
                    if(browser_.finishPush(gestureMs)){
                        // Source kind19 LOAD is a service intent, not a MIDI
                        // backend binding. Never fabricate a sent LOAD or ACK.
                        ++browserLoadIntents_;
                        lastBrowserLoadDeck_=browser_.deck();
                        browserLoadRequest_={browser_.deck(),browserSelectionSequence_,seq};
                        browserLoadPending_=true;
                        record(seq,sample,Stage::Decision,Reason::BrowserLoadIntent,0,data,3,lastBrowserLoadDeck_);
                    }
                    core_.observeBrowserGuardForDiagnostic(false,0,0);
                }
            }
            if(mutePageEnabled_&&accepted&&(cmd==0x90||cmd==0x80)&&physical>=0){
                muteGate_=muteNext; // commit time qualification only after accepted input
                const bool down=cmd==0x90&&data[2]!=0;
                if(physical==1){
                    if(down)muteGate_.beginBot2(gestureMs,core_.s3().physicalOwner(1)==1);
                    else muteGate_.releaseBot2(gestureMs);
                }else if(physical>=4&&physical<20){
                    const unsigned base=static_cast<unsigned>(physical-4);
                    if(down)muteGate_.beginBase(base,gestureMs,muteBaseOwned);
                    else muteGate_.releaseBase(base,gestureMs,muteBridge_);
                }
                // Any second BOT starts an S2 combo or concurrent BOT gesture.
                // In that case BOT2 is no longer entitled to a solo MUTE short/long.
                if(down&&physical<4&&physical!=1&&muteGate_.botHeld()&&core_.s3().physicalOwner(1)==30)muteGate_.cancelBot2();
            }
            if(accepted && localSampleBot2Release && !localSampleBot2Consumed &&
               mutePageEnabled_ && !muteGate_.botHeld()) {
                // An unconsumed BOT2 candidate opens MUTE on physical UP,
                // never by a premature 200ms LONG. It is an explicit SHORT.
                muteGate_.beginBot2(gestureMs,true);
                muteGate_.releaseBot2(gestureMs);
            }
            if(accepted && !wasMuteVisible && mutePageVisible() && (navMode()==2 || navMode()==4))
                requestDjLoopStop(true);
            if(accepted&&(cmd==0x90||cmd==0x80)&&physical>=32&&physical<36&&wasHeld!=(cmd==0x90&&data[2]!=0)){
                const bool down=cmd==0x90&&data[2]!=0;
                const auto topIndex=static_cast<std::size_t>(physical-32);
                if(down){
                    topSaveSuppress_[topIndex]=false;
                    if(topIndex<3){
                        const int candidateMode=core_.localSaveCandidateMode();
                        if(candidateMode==3||candidateMode==7){
                            // An accepted local candidate owns this TOP even
                            // if its NAV/profile preflight fails: never fall
                            // back to RECALL or reinterpret the BOT release.
                            topSaveSuppress_[topIndex]=true;
                            const bool admitted=core_.consumeLocalSaveCandidate(candidateMode);
                            if(!admitted)core_.declineLocalSaveCandidate();
                            const bool saved=admitted&&
                                (candidateMode==3?saveSamplePatchFromLive(static_cast<int>(topIndex)):
                                 saveMachinePatchFromLive(static_cast<int>(topIndex)));
                            if(saved)++sampleSavesAccepted_;
                            else ++sampleSavesRefused_;
                            if(candidateMode==3)muteGate_.cancelBot2();
                            record(seq,sample,Stage::Decision,
                                   saved?Reason::Musical:Reason::UnknownLive,
                                   0,data,3,saved?static_cast<int>(topIndex):-1);
                        }
                    }
                    topHold_[topIndex]=topSaveSuppress_[topIndex]?TopCapture{}:TopCapture{
                        core_.nativeNav().active().route(physical),
                        core_.nativeNav().active().contextMode,
                        core_.nativeNav().revision(),core_.nativeNav().active().at(291)
                    };
                }else{
                    if(topSaveSuppress_[topIndex]){
                        topSaveSuppress_[topIndex]=false;
                        topHold_[topIndex]={};
                        return {};
                    }
                    const auto captured=topHold_[static_cast<std::size_t>(physical-32)];
                    topHold_[static_cast<std::size_t>(physical-32)]={};
                    // TOP is a RESET request. Without authenticated RESET_DEFAULT
                    // it must never emit an invented zero or an arbitrary CC.
                    // Store an observable deferred intent only if the captured
                    // physical destination has not changed during the gesture.
                    const auto current=core_.nativeNav().active().route(physical);
                    if(captured.route.address>=0&&captured.route==current&&
                       captured.navRevision==core_.nativeNav().revision()&&
                       captured.context==core_.nativeNav().active().contextMode){
                        const auto index=static_cast<std::size_t>(physical-32);
                        // Source GOLDEN TOP dispatch. Root 0 = INST/DRUMS,
                        // root 1 = YELLOW/FILL. Source S3 @zf flags 303/304
                        // and page fields 297/298 qualify the selected voice.
                        // Exactly one owner is visible on a committed NAV
                        // image. TOP4 is invariant FILTER GLOBAL: never pulse.
                        const auto& nav=core_.nativeNav().active();
                        const int root=nav.at(291);
                        const bool normal=nav.at(292)==0 && nav.contextMode==0 &&
                            nav.presetFlag==0 && !mutePageVisible() &&
                            !core_.longNavPending() && (root==0||root==1);
                        TopPulseFamily family=TopPulseFamily::Init;
                        unsigned member=0;bool isPulse=false;
                        if(normal){
                            const int selected=nav.at(303+root);
                            if(selected==1 && index==2) {
                                const int voice=nav.at(297+root);
                                if(voice>=1 && voice<=8){
                                    family=root==0?TopPulseFamily::Drums:TopPulseFamily::Fill;
                                    member=static_cast<unsigned>(voice-1);isPulse=true;
                                }
                            }else if(selected==0){
                                if(root==0 && index<3){family=TopPulseFamily::Init;member=static_cast<unsigned>(index);isPulse=true;}
                                else if(root==1 && index<2){family=TopPulseFamily::Yellow;member=static_cast<unsigned>(index);isPulse=true;}
                            }
                        }
                        if(isPulse){
                            if(topPulse_.queue(family,member))
                                record(seq,sample,Stage::Decision,Reason::Musical,0,data,3,
                                       680+static_cast<int>(family==TopPulseFamily::Init?member:
                                       family==TopPulseFamily::Drums?4+member:
                                       family==TopPulseFamily::Yellow?12+member:14+member));
                            else record(seq,sample,Stage::Overflow,Reason::Rejected,0,data,3);
                        }else{
                        // Source ownership: SAMPLE(mode3) and MACHINE(mode7)
                        // TOP1..3 are PATCH recalls, not FILTER RESET.
                        // TOP4 is invariant FILTER GLOBAL RESET.
                        const bool patch=(physical!=35&&(captured.context==3||captured.context==7));
                        topIntents_[index]={physical,captured.route,seq,patch?TopAction::PatchRecall:TopAction::FilterReset};
                        topIntentValid_[index]=true;
                        if(patch && captured.context==3)armSamplePatch(index);
                        else if(patch && captured.context==7)armMachinePatch(index);
                        if(patch)++topPatchRecallsDeferred_;
                        else ++topResetsDeferred_;
                        record(seq,sample,Stage::Decision,Reason::TopResetDeferred,0,data,3,captured.route.address);
                        }
                    }
                }
            }
            if(accepted&&(cmd==0x90||cmd==0x80)&&physical>=20&&physical<32){
                const bool down=cmd==0x90&&data[2]!=0;
                // S2 advances before input: a LONG in this packet must qualify NAV first.
                const auto captured=core_.nativeNav().active().route(physical);
                if(wasHeld!=down)return spot(physical,down,captured,gestureMs,seq,sample);
            }
            if(accepted&&cmd==0xB0){
                if(data[1]<20)return continuous(data[1],data[2],gestureMs,seq,sample);
                const auto turn=decodeDjTurn(data[1],data[2]);
                if(turn.valid()){
                    record(seq,sample,Stage::Gesture,Reason::DjRelative,0,data,3,turn.delta);
                    // The S2->S3 resolver has already run; role10 (BOT3+DJ2
                    // rotation) is only valid if it captured physical owner40.
                    // Capture this one gesture before any MIDI callback result.
                    if(data[1]==21&&core_.surface().held(2)&&
                       core_.s3().physicalOwner(2)==40&&core_.djLoopRotationAllowed())dj2PushRotCaptured_=true;
                    // S7 source API: kind16 / address 947 (DJ1), 948 (DJ2).
                    // Two's-complement RAW is forwarded without pickup or CC de-dup.
                    // This is the virtual control API, NOT an assumed audio backend.
                    const int api=947+static_cast<int>(data[1])-20;
                    auto staged=core_.output();
                    if(!staged.apply({16,api,data[2],127})||staged.emitted()!=1)return {};
                    const auto p=staged.packet(0);
                    Result out{true,{p.status,p.number,p.value},seq,sample,true,api,data[2],16};
                    out.gestureMs=gestureMs;
                    out.dj2Qualified=data[1]==21&&core_.surface().held(2)&&
                        core_.s3().physicalOwner(2)==40&&core_.djLoopRotationAllowed();
                    record(seq,sample,Stage::Decision,Reason::Musical,0,out.bytes,3);
                    return out;
                }
            }
            return {};
        }
        // Explicit T01 bench profile only: never remap CH1 in the default AU.
        if(!historicalL02Bench_ || data[0]!=0xB0 || (data[1]!=4 && data[1]!=8)) {
            record(seq,sample,Stage::Decision,Reason::Rejected,cable,data,length);return {};
        }
        Result r{true,{0xB0,static_cast<uint8_t>(data[1]==4?74:71),data[2]},seq,sample};
        record(seq,sample,Stage::Decision,Reason::Mapped,cable,r.bytes,3);
        return r;
    }
public:
    // Lifecycle-only restore: call before rendering starts, never from UI during render.
    bool restoreSessionBeforeRendering(const l01::LiveState::SessionImage& image) noexcept {
        return !started_&&core_.restoreNativeSessionForDiagnostic(image);
    }
    // Lifecycle-only bank input, distinct from file selection/loading and from
    // an observed instrument. New banks are empty; no automatic legacy import.
    bool storeSamplePatchBeforeRendering(int owner,int slot,int config,
            const l01::BankS6::PatchImage& image,uint32_t expectedRevision) noexcept {
        if(started_ || owner<0 || owner>=8 || slot<0 || slot>=3 || config<1 ||
           config!=core_.nativeLive().sessionConfig() || !l01::BankS6::validatePatch(owner,image))return false;
        if(!core_.nativeBank().initialized() && !core_.initializeEmptyNativeBanksForDiagnostic())return false;
        return core_.saveNativePatchForDiagnostic(owner,slot,config,image,expectedRevision);
    }
    bool samplePatchPending() const noexcept {return samplePatch_.owner>=0||pairedTopSource_>=0;}
    Result samplePatchOnGrid(const GridBatch& batch,int64_t bufferStart) noexcept {
        if(!batch.valid || batch.overflow || batch.count>batch.events.size())return {};
        for(unsigned i=0;i<batch.count;++i){
            const auto& edge=batch.events[i];
            // HostGrid suppresses stale edges and creates a fresh anchor on seek.
            // A smaller ordinal may therefore be a genuine BAR after a rewind.
            if(edge.kind!=GridKind::Bar || edge.ordinal<0 || edge.ordinal==lastSamplePatchBar_)continue;
            lastSamplePatchBar_=edge.ordinal;
            if(samplePatch_.owner<0)continue;
            const auto request=samplePatch_;samplePatch_={};
            const int base=request.machine?320+42*request.owner:40*request.owner;
            const int width=request.machine?42:40;
            // Never override active SPOT owners, especially momentary OFF debts.
            for(const auto& hold:spots_)if(hold.cell>=base&&hold.cell<base+width)return {};
            std::array<int,30> values{};
            const int count=request.machine?30:28;
            int config=-1;
            if(request.machine){
                S8MachinePatchBank::Patch patch{};
                if(!s8Machine_.read(static_cast<unsigned>(request.owner),
                                    static_cast<unsigned>(request.slot),patch)||
                   !patch.valid||patch.revision!=request.revision||
                   patch.config!=core_.nativeLive().sessionConfig())return {};
                values=patch.values;config=patch.config;
                if(!S8MachinePatchBank::validate(patch.values))return {};
            }else if(request.s8){
                S8SamplePatchBank::Patch patch{};
                if(!s8Sample_.read(static_cast<unsigned>(request.owner),
                                   static_cast<unsigned>(request.slot),patch)||
                   !patch.valid||patch.revision!=request.revision||
                   patch.config!=core_.nativeLive().sessionConfig())return {};
                for(int field=0;field<28;++field)values[static_cast<std::size_t>(field)]=patch.values[static_cast<std::size_t>(field)];
                config=patch.config;
                if(!S8SamplePatchBank::validate(patch.values))return {};
            }else{
                // Historical S6 pre-render fixture path, explicitly isolated
                // from the S8 banks used by actual physical SAVE.
                l01::BankS6::PatchRecord patch{};
                if(!core_.readNativePatch(request.owner,request.slot,patch)||!patch.valid||
                   patch.revision!=request.revision||
                   patch.config!=core_.nativeLive().sessionConfig())return {};
                for(int field=0;field<28;++field)values[static_cast<std::size_t>(field)]=patch.values[static_cast<std::size_t>(field)];
                config=patch.config;
                S8SamplePatchBank::Values prior{};
                for(int field=0;field<28;++field)prior[static_cast<std::size_t>(field)]=values[static_cast<std::size_t>(field)];
                if(!S8SamplePatchBank::validate(prior))return {};
            }
            auto projection=core_.output();projection.drain();
            Result result{};
            const auto seq=++sequence_;
            const auto sample=bufferStart+edge.offset;
            for(int field=0;field<count;++field){
                const int type=field<(request.machine?18:16)?10:11;
                const int api=request.machine?
                    (field<18?128+18*request.owner+field:96+12*request.owner+field-18):
                    (field<16?16*request.owner+field:12*request.owner+field-16);
                const int value=values[static_cast<std::size_t>(field)];
                if(!projection.apply({type,api,value,127}))return {};
                // OutputS4 deduplicates CC/state writes. Preserve its exact bytes.
                for(std::size_t n=0;n<projection.emitted();++n){
                    const auto packet=projection.packet(n);
                    if(!result.emit)result={true,{packet.status,packet.number,packet.value},seq,sample,true,api,value,type};
                    else {
                        if(result.extraCount>=result.extra.size())return {};
                        result.extra[result.extraCount++]={{packet.status,packet.number,packet.value},api,value,type,127,false};
                    }
                }
                projection.drain();
            }
            if(request.machine){
                if(!core_.recallNativeS8MachinePatch(request.owner,values,config,
                                                   core_.nativeLive().revision()))return {};
            }else if(request.s8){
                S8SamplePatchBank::Values patchValues{};
                for(int field=0;field<28;++field)patchValues[static_cast<std::size_t>(field)]=values[static_cast<std::size_t>(field)];
                if(!core_.recallNativeS8SamplePatch(request.owner,patchValues,config,
                                                  core_.nativeLive().revision()))return {};
            }else if(!core_.recallNativeSamplePatch(request.owner,request.slot,
                                                     request.revision,
                                                     core_.nativeLive().revision()))return {};
            return result;
        }
        return {};
    }
    bool requestDocumentRestore(const DocumentImage& image) noexcept {
        auto candidate=image;
        if(!validDocument(candidate)||requestedToken_==UINT32_MAX)return false;
        documents_->ui={candidate,++requestedToken_};restoreEpoch_.store(requestedToken_,std::memory_order_release);
        documents_->restores.publish(documents_->ui);return true;
    }
    DocumentImage documentForHost() noexcept {
        DocumentFrame next{};
        if(documents_->snapshots.take(next)&&next.token>=requestedToken_)documents_->ui=next;
        return documents_->ui.image;
    }
    // UI/host thread only, serialized externally. Do not call core from these methods.
    bool requestSessionRestore(const SessionImage& image) noexcept {
        if(!validSession(image)||requestedToken_==0xFFFFFFFFu)return false;
        uiSession_={image,++requestedToken_};
        documents_->ui.image.live=image;documents_->ui.image.current.live=image;
        for(unsigned c=0;c<image.desired.size();++c)if(!GlobalSnapshot::inScope(c))documents_->ui.image.current.live.desired[c]=-2;
        documents_->ui.image.current.memoryEngaged.fill(false);documents_->ui.image.current.presetEngaged.fill(false);
        restoreEpoch_.store(requestedToken_,std::memory_order_release);restores_.publish(uiSession_);return true;
    }
    SessionImage sessionForHost() noexcept {
        SessionFrame next{};
        if(snapshots_.take(next)&&next.token>=requestedToken_)uiSession_=next;
        return uiSession_.image;
    }
    // Render boundaries are the sole application/publication points.
    void beginRenderBuffer() noexcept {
        muteLeds_.beginBuffer();
        if(ledRefreshRequest_.exchange(false,std::memory_order_acq_rel))muteLeds_.invalidate();
        started_=true;
        const int faceRequest=presetFaceRequest_.exchange(-1,std::memory_order_acq_rel);
        if(faceRequest==1)showPresetPage();else if(faceRequest==0)leavePresetPage();
        if(alternativeFlight_.valid&&alternativeFlight_.waiting==0&&!restores_.hasUpdate()&&!documents_->restores.hasUpdate())finishAlternativeChange();
        else if(transportStopped_&&!alternativeFlight_.valid&&!restores_.hasUpdate()&&!documents_->restores.hasUpdate()){
            bool pending=false;for(int target:alternatives_.pending())if(target>=0)pending=true;
            if(pending)beginAlternativeChange();
        }
        // A session must not replace an active SOFT owner while its MIDI
        // NoteOff may still be due (this debt is outside OutputS4).
        if((!restores_.hasUpdate()&&!documents_->restores.hasUpdate())||core_.nativeNav().hasStage()||core_.output().outstanding()!=0||soft_.hasOutstandingNotes()||topPulse_.hasOwedOff())return;
        for(std::size_t i=0;i<36;++i)if(core_.surface().held(i))return;
        DocumentFrame document{};
        if(documents_->restores.take(document)&&document.token==restoreEpoch_.load(std::memory_order_acquire)){
            if(applyDocument(document.image)){appliedToken_=document.token;documentDirty_=true;}
        }
        SessionFrame next{};
        if(restores_.take(next)&&next.token==restoreEpoch_.load(std::memory_order_acquire)&&core_.restoreNativeSessionForDiagnostic(next.image)){
            appliedToken_=next.token;pickup_=ContinuousPickup{};
            core_.output()=l01::OutputS4{};core_.enableTestMidiProjection(true);
            samplePatch_={};pairedTopSource_=-1; // a restored session cannot execute an old queued recall
            musicalScheduler_.stop();sourcePending_.fill(-1);sourceStatus_.fill(SourceRecallStatus::None);
            for(auto& pending:memoryPending_)pending={};
            memoryEngaged_.fill(false);presetEngaged_.fill(false);presetValid_.fill(false);
            alternativeFlight_={};alternatives_.stop();alternativeBaseValid_.fill(false);
            pendingPresetSlot_=-1;initPending_=false;redActive_=false;redImage_.valid=false;redHeld_=false;
            sourceRead_=sourceWrite_;sourceBlocked_=false;cueLoad_.stop();
        }
    }
    void endRenderBuffer() noexcept {
        publishedPresetFace_.store(core_.nativeNav().active().at(292)==1&&core_.nativeNav().active().presetFlag==1&&!mutePageVisible(),std::memory_order_release);
        const auto revision=core_.nativeLive().revision();
        if(documentDirty_||revision!=publishedRevision_||appliedToken_!=publishedToken_){
            documents_->snapshots.publish({captureDocument(),appliedToken_});documentDirty_=false;
        }
        if(revision==publishedRevision_&&appliedToken_==publishedToken_)return;
        snapshots_.publish({core_.nativeLive().snapshot(),appliedToken_});
        publishedRevision_=revision;publishedToken_=appliedToken_;
    }
    l01::LiveRead desiredLive(int cell) const noexcept {return core_.nativeLive().desired(cell);}
    // Called immediately before each callback; dependent FLASH ON is suppressed
    // unless its nominal OFF succeeded. Its own OFF still follows a failed ON.
    bool canSend(const Result& r) const noexcept {
        if(r.musical&&((r.type==11&&r.value==1)||r.type==12||r.type==14||r.type==16)&&r.sequence!=sequence_)return false;
        if(r.type==12||r.type==14){
            if(r.requiresOff&&(confirmedOffSequence_!=r.sequence||confirmedOffApi_!=r.api))return false;
            auto draft=core_.output();return draft.apply({r.type,r.api,1,r.key});
        }
        if(r.type==13)return core_.output().owner(r.key)==r.api&&core_.output().debt(r.api);
        return true;
    }
    void sent(const Result& r,int32_t status) noexcept {
        auto& output=core_.output();
        if(r.musical){
            if(r.type==12||r.type==14){
                // The callback may have partly delivered a failing ON.
                if(canSend(r)){
                    output.apply({12,r.api,1,r.key});
                    if(r.type==12)output.apply({11,r.api,1,127});
                }
            }else if(r.type==13){
                if(output.owner(r.key)==r.api){
                    output.apply({11,r.api,0,127});
                    if(status==0)output.apply({13,r.api,0,r.key});
                }
            }else if(status==0||(r.type==11&&r.value==1))output.apply({r.type,r.api,r.value,127});
            output.drain();
            if(status==0&&r.type==11&&r.value==0){confirmedOffSequence_=r.sequence;confirmedOffApi_=r.api;}
            // Remove an acknowledged STOP debt permanently: a later new ON
            // at this address must not be mistaken for the old stopped note.
            if(status==0&&(r.type==13||(r.type==11&&r.value==0))&&
               r.api>=0&&r.api<702&&stopAwaiting_[static_cast<std::size_t>(r.api)]&&
               !output.debt(r.api)){
                stopAwaiting_[static_cast<std::size_t>(r.api)]=false;
                --stopPendingCount_;
            }
        }
        // Do not arm Browser on a failed or unsent relative rotation:
        // there is no verified track selection for DJ1 PUSH to confirm.
        if(status==0&&r.musical&&r.type==16&&r.api==948&&r.sequence==sequence_&&
           r.dj2Qualified&&core_.surface().held(2)&&core_.s3().physicalOwner(2)==40){
            // A qualified push+rotation, unlike a bare DJ2 turn, has a
            // captured gesture owner. Count requested length changes only;
            // actual DJ loop playback requires authenticated backend ACKs.
            if(r.dj2Qualified){
                ++dj2QualifiedTurns_;
                dj2LastQualifiedDelta_=decodeDjTurn(21,static_cast<unsigned>(r.value)).delta;
                // Q6: an admitted BOT3+DJ2 ROT requests activation if
                // currently OFF. This is *requested*, never an audio ACK.
                if(dj2LoopObserved_!=Dj2LoopObserved::On&&!dj2LoopStartRequested_){
                    dj2LoopStartRequested_=true;
                    ++dj2BackendGeneration_;
                    djLoopCommand_=DjLoopCommand::Start;
                    ++dj2LoopRequests_;
                }
            }
        }
        if(status==0&&r.musical&&r.type==16&&r.api==947&&r.sequence==sequence_&&
           browser_.observeTurn(decodeDjTurn(20,static_cast<unsigned>(r.value)).delta,r.gestureMs)){
            browserSelectionSequence_=r.sequence;
            browserLoadPending_=false; // old LOAD cannot target a newer selection
            core_.observeBrowserGuardForDiagnostic(true,browser_.deck(),browser_.deadlineMs());
        }
        record(r.sequence,r.sample,Stage::TxOut,status==0?Reason::Sent:Reason::SendFailed,0,r.bytes,3,status);
    }
    void noOutput(const Result& r) noexcept {
        if(r.type==13&&core_.output().owner(r.key)==r.api){
            core_.output().apply({11,r.api,0,127});core_.output().drain();
        }
        record(r.sequence,r.sample,Stage::Decision,Reason::NoOutput,0,r.bytes,3);
    }
    // Re-arm the scan once per render buffer. A callback failure or absent
    // MIDI output must never consume a pending STOP NoteOff permanently.
    void beginStopRetryPass(int64_t sample) noexcept {stopRead_=0;stopSample_=sample;}
    unsigned pendingStopOffs() const noexcept {return stopPendingCount_;}
    void stop(int64_t sample) noexcept { muteLeds_.cancelFlashes(); documentDirty_=true;
        started_=true;transportStopped_=true;presetFaceRequest_.store(-1,std::memory_order_release);
        alternativeFlight_={};alternatives_.stop();alternativeBaseValid_.fill(false);
        for(auto& selector:alternativeSelectors_)selector={};
        redActive_=false;redImage_.valid=false;redHeld_=false;pendingPresetSlot_=-1;initPending_=false;
        // Keep output debts until the real callback confirms each OFF.
        const auto output=core_.output();
        stopCount_=stopRead_=stopPendingCount_=0;stopSample_=sample;
        stopAwaiting_.fill(false);
        for(int a=0;a<702;++a)if(output.debt(a)){
            stopAddresses_[stopCount_++]=a;
            stopAwaiting_[static_cast<std::size_t>(a)]=true;
            ++stopPendingCount_;
        }
        muteGate_.stop();muteBridge_.stop();browser_.stop();dj2PushRotCaptured_=false;
        // STOP requests DJ LOOP termination without claiming audio ACK.
        requestDjLoopStop(false,true);
        for(auto& target:topHold_)target={};
        topIntentValid_.fill(false);
        samplePatch_={};pairedTopSource_=-1;lastSamplePatchBar_=-1;
        groupDownEpoch_.fill(0); // STOP invalidates stale GROUP proof
        for(auto& hold:groupHolds_)hold={}; // no ghost long on later UP
        soft_.stop(); // cancels only uncommitted SOFT; applied pages persist
        musicalScheduler_.stop();
        for(auto& pending:memoryPending_)pending={};
        sourcePending_.fill(-1);sourceRead_=sourceWrite_;sourceBlocked_=false;
        for(auto& status:sourceStatus_)if(status==SourceRecallStatus::Pending)status=SourceRecallStatus::None;
        cueLoad_.stop(); // cancel pending BAR intents, preserve actual CC-sent state
        topPulse_.stop(); // cancels unattempted pulses; retains every attempted OFF
        topSaveSuppress_.fill(false);
        browserLoadPending_=false;browserSelectionSequence_=0;
        core_.observeBrowserGuardForDiagnostic(false,0,0);
        core_.onHostStop(); // must not fabricate UP or clear held keys
        core_.output()=output;
        for(auto& held:spots_){
            if(held.cell>=0&&(held.mode==3||(held.mode==1&&held.previous==0)))
                core_.commitNativeLive(held.cell,0,core_.nativeLive().revision());
            held.cell=-1;
        }
        record(++sequence_,sample,Stage::Stop,stopCount_?Reason::Musical:Reason::SurfaceOnly,0,nullptr,0);
    }
    Result nextStopPacket() noexcept {
        if(!stopPendingCount_)return {};
        while(stopRead_<stopCount_){
            const int a=stopAddresses_[stopRead_++];
            if(!stopAwaiting_[static_cast<std::size_t>(a)])continue;
            if(!core_.output().debt(a)){
                stopAwaiting_[static_cast<std::size_t>(a)]=false;
                --stopPendingCount_;
                continue;
            }
            Result r{true,{static_cast<uint8_t>(0x86+a/128),static_cast<uint8_t>(a%128),0},sequence_,stopSample_,true,a,0,11};
            for(int key=0;key<36;++key)if(core_.output().owner(key)==a){r.type=13;r.key=key;break;}
            return r;
        }
        return {};
    }
    int observed() const noexcept {return -2;} // no authenticated backend profile in L02
    bool surfaceHeld(size_t i) const noexcept {return core_.surface().held(i);}
    // Render-thread-only inspection; UI reads the SPSC trace, never core state.
    int navMode() const noexcept {return core_.nativeNav().active().at(292);}
    bool longNavPending() const noexcept {return core_.longNavPending();}
    void advance(int64_t ms,int64_t sample) noexcept { documentDirty_=true;
        const auto signatureBeforeTimer=presetSignature();
        started_=true;
        const bool wasMuteVisible=mutePageVisible();
        muteGate_.advance(ms);
        if(!wasMuteVisible && mutePageVisible() && (navMode()==2 || navMode()==4))requestDjLoopStop(true);
        // Source GOLDEN long SPOT threshold: 120ms. The host owns this
        // qualification, not a second BAR/BEAT scheduler. First valid
        // opportunity emits once, at/after 120ms; an UP at the threshold
        // uses the exact same source proof.
        for(auto& hold:groupHolds_)fireGroupLong(hold,ms);
        if(!core_.gesture().timerNeeded())return;
        const auto seq=++sequence_;
        const int previousNav=navMode();
        core_.retainDjLoopPage(dj2LoopObserved_!=Dj2LoopObserved::Off || dj2LoopStartRequested_);
        if(!core_.onTimer(ms))record(seq,sample,Stage::Decision,Reason::ClockRejected,0,nullptr,0);
        else resolveDjLoopNavigation(previousNav);
        invalidatePresetsIfChanged(signatureBeforeTimer);
        drainCore(seq,sample);
    }
    // SPSC: exactly one consumer (UI main thread); no reset while rendering.
    bool pop(Trace& t) noexcept {
        const auto r=read_.load(std::memory_order_relaxed);
        if(r==write_.load(std::memory_order_acquire))return false;
        t=trace_[r%TraceCapacity];read_.store(r+1,std::memory_order_release);return true;
    }
    uint32_t dropped() const noexcept {return dropped_.load(std::memory_order_relaxed);}
    void renderOverflow(int64_t sample) noexcept {
        overflows_.fetch_add(1,std::memory_order_relaxed);
        record(++sequence_,sample,Stage::Overflow,Reason::Rejected,0,nullptr,0);
    }
    uint32_t overflows() const noexcept {return overflows_.load(std::memory_order_relaxed);}
private:
    // S8 SAMPLE: capture 28 instrument parameters only, excluding modes
    // 28..39, which belong solely to the S8 configuration.
    bool saveSamplePatchFromLive(int slot) noexcept {
        if(redActive_)return false;
        const auto& nav=core_.nativeNav();
        if(slot<0||slot>=3||!nav.stable()||nav.hasStage()||nav.active().at(292)!=3 ||
           nav.active().contextMode!=3||core_.longNavPending()||samplePatchPending())
            return false;
        const int owner=nav.active().at(313)-1;
        const int config=core_.nativeLive().sessionConfig();
        if(owner<0||owner>=8||config<1||config>127)return false;
        for(const auto& spot:spots_)if(spot.cell>=40*owner&&spot.cell<40*(owner+1))return false;
        S8SamplePatchBank::Values image{};image.fill(S8SamplePatchBank::Unknown);
        for(unsigned field=0;field<S8SamplePatchBank::ParamCount;++field){
            const auto value=core_.nativeLive().desired(40*owner+static_cast<int>(field));
            if(!value.known)return false;
            image[field]=value.value;
        }
        if(!S8SamplePatchBank::validate(image))return false;
        S8SamplePatchBank::Patch prior{};
        if(!s8Sample_.read(static_cast<unsigned>(owner),static_cast<unsigned>(slot),prior) ||
           (prior.valid&&prior.config!=config))return false;
        return s8Sample_.save(static_cast<unsigned>(owner),static_cast<unsigned>(slot),
                              config,image,prior.revision);
    }
    // Native S8 MACHINE capture: 18 CC + 12 SPOT states. The 12
    // behavior modes at offset 30..41 are immutable to PATCH operations.
    bool saveMachinePatchFromLive(int slot) noexcept {
        if(redActive_)return false;
        const auto& nav=core_.nativeNav();
        if(slot<0||slot>=3||!nav.stable()||nav.hasStage()||nav.active().at(292)!=7||
           nav.active().contextMode!=7||nav.active().presetFlag!=0||
           core_.longNavPending()||samplePatchPending())return false;
        const int selected=nav.active().at(314);
        const int config=core_.nativeLive().sessionConfig();
        if(selected<1||selected>24||config<1||config>127)return false;
        // S7 pair authority, retained by S8: the first eight pages share
        // four PATCH60 banks. A BOT3 save on either page captures both.
        if(selected<=8)return saveSourcePatch(static_cast<unsigned>((selected-1)/2),static_cast<unsigned>(slot));
        const int owner=selected-1,base=320+42*owner;
        for(const auto& hold:spots_)if(hold.cell>=base&&hold.cell<base+42)return false;
        S8MachinePatchBank::Values values{};values.fill(S8MachinePatchBank::Unknown);
        for(unsigned i=0;i<S8MachinePatchBank::ParamCount;++i){
            const auto value=core_.nativeLive().desired(base+static_cast<int>(i));
            if(!value.known)return false;
            values[i]=value.value;
        }
        if(!S8MachinePatchBank::validate(values))return false;
        S8MachinePatchBank::Patch prior{};
        if(!s8Machine_.read(static_cast<unsigned>(owner),static_cast<unsigned>(slot),prior)||
           (prior.valid&&prior.config!=config))return false;
        return s8Machine_.save(static_cast<unsigned>(owner),static_cast<unsigned>(slot),
                               config,values,prior.revision);
    }
    struct GroupHold {
        QualifiedGroupAction action{};
        std::int64_t downMs{};
        bool active{},fired{};
    };
    static constexpr std::int64_t GroupLongMs=120;
    void fireGroupLong(GroupHold& hold,std::int64_t now,bool acceptedPhysicalRelease=false) noexcept {
        if(!hold.active||hold.fired||now<hold.downMs ||
           now-hold.downMs<GroupLongMs)return;
        // Explicit owner, original NAV epoch, currently held key, and
        // no competing MUTE/overlays are required on every attempt.
        if(mutePageVisible())return;
        const bool allowed=acceptedPhysicalRelease ?
            (hold.action.physical<groupDownEpoch_.size() &&
             groupDownEpoch_[hold.action.physical]==hold.action.physicalDownEpoch &&
             GroupActionGate::authorize(core_.nativeNav(),hold.action,true,
                                       core_.longNavPending())) :
            (hold.action.physical<groupDownEpoch_.size() &&
             groupDownEpoch_[hold.action.physical]==hold.action.physicalDownEpoch &&
             acceptQualifiedGroupAction(hold.action));
        if(allowed && (acceptedPhysicalRelease?
                       topPulse_.queueGroup(hold.action.fill,hold.action.page,hold.action.action):true)){
            hold.fired=true;
            record(++sequence_,now,Stage::Gesture,Reason::Musical,0,nullptr,0,
                (hold.action.fill?632:608)+
                (hold.action.fill?6:3)*static_cast<int>(hold.action.page)+
                static_cast<int>(hold.action.action));
        }
    }
    std::array<GroupHold,12> groupHolds_{};
    struct SamplePatchRequest {int owner{-1},slot{-1};uint32_t revision{};bool s8{},machine{};};
    SamplePatchRequest samplePatch_{};int pairedTopSource_{-1};int64_t lastSamplePatchBar_{-1};
    void armSamplePatch(unsigned index) noexcept {
        if(samplePatchPending() || index>=3 || !topIntentValid_[index])return;
        const auto& intent=topIntents_[index];
        const int packed=intent.route.address;
        if(packed<0 || packed/1024!=11 || (packed%1024)/32!=1)return;
        const int owner=packed%32-1,slot=intent.route.position;
        if(owner<0||owner>=8||slot<0||slot>=3)return;
        S8SamplePatchBank::Patch native{};
        if(s8Sample_.read(static_cast<unsigned>(owner),static_cast<unsigned>(slot),native) &&
           native.valid && native.config==core_.nativeLive().sessionConfig()) {
            samplePatch_={owner,slot,native.revision,true};
        }else{
            // Explicit pre-render S6 fixtures remain available for historical
            // compatibility tests; normal physical SAVE never writes S6.
            l01::BankS6::PatchRecord historical{};
            if(!core_.readNativePatch(owner,slot,historical)||!historical.valid||
               historical.config!=core_.nativeLive().sessionConfig())return;
            samplePatch_={owner,slot,historical.revision,false};
        }
        topIntentValid_[index]=false; // command accepted once; BAR still has not applied it
    }
    void armMachinePatch(unsigned index) noexcept {
        if(samplePatchPending()||index>=3||!topIntentValid_[index])return;
        const auto& intent=topIntents_[index];
        const int packed=intent.route.address;
        if(packed<0||packed/1024!=11)return;
        // MACHINE TOP records use the S3 selected target captured at DOWN;
        // never infer a Roland program number from a TOP button.
        const auto& nav=core_.nativeNav().active();
        if(nav.at(292)!=7||nav.contextMode!=7||nav.presetFlag!=0)return;
        const int selected=nav.at(314);
        const int slot=intent.route.position;
        if(selected<1||selected>24||slot<0||slot>=3)return;
        if(selected<=8){
            const unsigned source=static_cast<unsigned>((selected-1)/2);
            SourcePatchBank::Patch patch{};
            if(!readSourcePatch(source,static_cast<unsigned>(slot),patch)||patch.config!=core_.nativeLive().sessionConfig())return;
            if(!requestSourceRecall(source,static_cast<unsigned>(slot)))return;
            pairedTopSource_=transportStopped_?-1:static_cast<int>(source);
            topIntentValid_[index]=false;return;
        }
        S8MachinePatchBank::Patch patch{};
        if(!s8Machine_.read(static_cast<unsigned>(selected-1),static_cast<unsigned>(slot),patch)||
           !patch.valid||patch.config!=core_.nativeLive().sessionConfig())return;
        samplePatch_={selected-1,slot,patch.revision,true,true};
        topIntentValid_[index]=false;
    }
    void requestDjLoopStop(bool reset,bool invalidate=false) noexcept {
        if(dj2LoopStartRequested_ || dj2LoopObserved_==Dj2LoopObserved::On){
            ++dj2LoopStopRequests_;
            if(reset || djLoopCommand_!=DjLoopCommand::StopReset)
                djLoopCommand_=reset?DjLoopCommand::StopReset:DjLoopCommand::Stop;
        }
        dj2LoopStartRequested_=false;
        ++dj2BackendGeneration_; // rejects late START observations after any STOP
        if(invalidate)dj2LoopObserved_=Dj2LoopObserved::Unknown;
    }
    void resolveDjLoopNavigation(int previous) noexcept {
        if(core_.djLoopStopPush())requestDjLoopStop(false);
        const bool wasDjPage=previous==2 || previous==4;
        const bool isDjPage=navMode()==2 || navMode()==4;
        if(wasDjPage && !isDjPage)requestDjLoopStop(true);
    }
    DjLoopCommand djLoopCommand_{DjLoopCommand::None};
    bool mutePageEnabled_{};
    l01::MutePhysicalGate muteGate_{};
    MuteBarBridge muteBridge_{};
    MuteMidiProjection muteMidi_{};
    MuteLedOutput muteLeds_{};std::uint64_t ledNavRevision_{};std::atomic<bool> ledRefreshRequest_{false};
    struct SpotHold {int cell{-1},api{-1},mode{},previous{};int64_t downMs{};};
    static int spotApi(int cell) noexcept {
        if(cell<320&&cell%40>=16&&cell%40<28)return 12*(cell/40)+cell%40-16;
        if(cell>=320&&cell<1328&&(cell-320)%42>=18&&(cell-320)%42<30)
            return 96+12*((cell-320)/42)+(cell-320)%42-18;
        if(cell>=1477&&cell<1513)return cell-985;
        if(cell>=1768&&cell<1864)return cell-1384;
        return -1;
    }
    Result spot(int physical,bool down,l01::NavRoute route,int64_t ms,uint32_t seq,int64_t sample) noexcept {
        auto& held=spots_[static_cast<std::size_t>(physical-20)];
        int cell=-1,api=-1,value=-1,mode=0,previous=0;
        if(down){
            held={};int preset=-1;
            // Source @w0 excludes d6=24; other owners are not guessed.
            if(route.address<0||route.address/1024!=3||
               !l01::LiveState::spotAddresses(route.address,route.position,cell,preset))return {};
            const auto live=core_.nativeLive().desired(cell);
            auto selected=preset>=0?core_.nativeLive().desired(preset):l01::LiveRead{true,1};
            if(cell>=0&&cell<320&&cell%40>=16&&cell%40<28){
                const int m=s8Sample_.mode(static_cast<unsigned>(cell/40),static_cast<unsigned>(cell%40-16));
                if(m>=1&&m<=4)selected={true,m};
            }else if(cell>=320&&cell<1328&&(cell-320)%42>=18&&(cell-320)%42<30){
                const int machine=(cell-320)/42;
                const int m=s8MachineMode(machine,(cell-320)%42-18);
                if(m>=1&&m<=4)selected={true,m};
                else if(machine<8&&activeAlternative(static_cast<unsigned>(machine/2))>=0)selected={};
            }
            api=spotApi(cell);
            if(!live.known||live.value>1||!selected.known||selected.value<1||selected.value>4||api<0)return {};
            mode=selected.value;previous=live.value;
            held={cell,api,mode,previous,ms};value=mode==4?0:1;
        }else{
            const auto capture=held;held={};
            if(capture.cell<0||capture.mode==4)return {};
            cell=capture.cell;api=capture.api;mode=capture.mode;previous=capture.previous;
            value=(mode==3||previous==1||(mode==1&&ms-capture.downMs>199))?0:1;
        }
        auto projection=core_.output();
        int kind=11;std::size_t nominalPackets=0;
        if(mode==3&&previous==0&&down){
            kind=12;
            if(!projection.apply({12,api,1,physical})){held={};return {};}
        }else if(mode==3&&previous==0&&!down&&projection.owner(physical)==api){
            // Nominal OFF cannot steal the TOUCH; its owed physical OFF follows.
            kind=13;
            if(!projection.apply({11,api,0,127})||!projection.apply({13,api,0,physical}))return {};
        }else{
            if(!projection.apply({11,api,value,127})){if(down)held={};return {};}
            nominalPackets=projection.emitted();
            if(mode==4)projection.apply({14,api,1,physical});
        }
        const bool committed=core_.commitNativeLive(cell,value,core_.nativeLive().revision());
        if(!committed){
            if(down)held={};
            // A bounded LIVE counter must not suppress an already owed release.
            if(down||value!=0||!core_.output().debt(api))return {};
        }
        if(!projection.emitted()){
            core_.output()=projection; // state-only nominal change, no MIDI debt erased
            return {};
        }
        Result r{};
        for(std::size_t n=0;n<projection.emitted();++n){
            const auto p=projection.packet(n);
            int effect=kind,logical=value,key=kind==11?127:physical;bool needsOff=false;
            if(mode==4&&n>=nominalPackets){
                effect=n==nominalPackets?14:13;logical=effect==14?1:0;key=physical;
                needsOff=effect==14&&nominalPackets>0;
            }
            if(n==0){r={true,{p.status,p.number,p.value},seq,sample,true,api,logical,effect};r.key=key;r.requiresOff=needsOff;}
            else{r.extra[n-1]={{p.status,p.number,p.value},api,logical,effect,key,needsOff};++r.extraCount;}
        }
        record(seq,sample,Stage::Decision,Reason::Musical,0,r.bytes,3);return r;
    }
    Result continuous(int cc,int raw,int64_t ms,uint32_t seq,int64_t sample) noexcept {
        const auto target=continuousTarget(core_.nativeNav().active().route(36+cc));
        if(!target.valid()||target.api==572)return {};
        const auto live=core_.nativeLive().desired(target.cell);
        // S3 @zr: a physical move is a real initial DESIRED value even when
        // the session has no fixture. It is not an inferred RESET default.
        // Seeding pickup with raw also latches this physical controller, so
        // the next movement does not spuriously wait for a second pickup.
        if(core_.nativeLive().revision()>=l01::LiveState::MaxRevision)return {};
        const int value=pickup_.consider(cc,target.api,live.known?live.value:raw,raw,ms);
        pickupLedFeedback(cc,ms);
        if(value<0){record(seq,sample,Stage::Decision,Reason::PickupWait,0,nullptr,0);return {};}
        const bool loop=(target.api>=569&&target.api<=571)||target.api==573;
        auto projection=core_.output();
        if(loop){
            // Compute every derived gain from one staged LIVE revision; m has no CC.
            auto multiplier=core_.nativeLive().desired(1341);
            if(target.cell==1341)multiplier={true,value};
            const int first=target.cell==1341?0:target.api-569;
            const int last=target.cell==1341?3:first+1;
            for(int i=first;i<last;++i){
                auto base=core_.nativeLive().desired(1337+i);
                if(target.cell==1337+i)base={true,value};
                if(base.known&&multiplier.known){
                    const int gain=(base.value*multiplier.value+63)/127;
                    if(!projection.apply({15,569+i,gain,127}))return {};
                }else record(seq,sample,Stage::Decision,Reason::UnknownLive,0,nullptr,0);
            }
        }else if(!projection.apply({10,target.api,value,127}))return {};
        if(!core_.commitNativeLive(target.cell,value,core_.nativeLive().revision()))return {};
        if(projection.emitted()==0)return {};
        Result r{};
        for(std::size_t i=0;i<projection.emitted();++i){
            const auto p=projection.packet(i);
            const int api=(static_cast<int>(p.status)-0xB0-6)*128+p.number;
            const int type=loop?15:10;
            if(i==0)r={true,{p.status,p.number,p.value},seq,sample,true,api,p.value,type};
            else {r.extra[i-1]={{p.status,p.number,p.value},api,p.value,type};++r.extraCount;}
        }
        record(seq,sample,Stage::Decision,Reason::Musical,0,r.bytes,3);
        return r;
    }
    void drainCore(uint32_t seq,int64_t sample) noexcept {
        for(size_t i=0;i<core_.traceCount();++i) {
            const auto e=core_.trace(i);
            if(e.kind==l01::GestureEventKind::Qualified) {
                const uint8_t detail[]{static_cast<uint8_t>(e.role),static_cast<uint8_t>(e.phase),
                    static_cast<uint8_t>(navMode())};
                record(seq,sample,Stage::Gesture,Reason::Qualified,0,detail,3);
            }
        }
        // Diagnostic consumer speed must never gate the musical input queue.
        core_.drainTrace();core_.drainS3Trace();
    }
    void record(uint32_t sequence,int64_t sample,Stage stage,Reason reason,uint8_t cable,
                const uint8_t* bytes,size_t length,int32_t result=0) noexcept {
        const auto w=write_.load(std::memory_order_relaxed);
        if(w-read_.load(std::memory_order_acquire)>=TraceCapacity) {
            dropped_.fetch_add(1,std::memory_order_relaxed);return;
        }
        Trace t{};t.sequence=sequence;t.sample=sample;t.stage=stage;t.reason=reason;t.cable=cable;
        t.length=static_cast<uint8_t>(length>255?255:length);t.result=result;
        if(bytes)for(size_t i=0;i<3 && i<length;++i)t.bytes[i]=bytes[i];
        trace_[w%TraceCapacity]=t;write_.store(w+1,std::memory_order_release);
    }
    l01::CoreCoordinatorV3 core_{};
    ContinuousPickup pickup_{};
    SoftMidiBridge soft_{};
    TopPulseBus topPulse_{};
    S8SamplePatchBank s8Sample_{};
    S8MachinePatchBank s8Machine_{};
    static int sourceForRoute(l01::NavRoute route) noexcept {
        if(route.address<0)return -1;
        const int family=(route.address%1024)/32,page=route.address%32;
        if(family==2&&page>=1&&page<=8)return (page-1)/2;
        if((family==3||family==4)&&page>=1&&page<=8)return 2*(family-3)+(page>4?1:0);
        if(family==13&&page==0&&route.position>=0&&route.position<4)return route.position;
        if(family==0&&page==0&&route.address/1024==0&&route.position>=0&&route.position<4)return route.position;
        return -1;
    }
    static int sourceForPacket(int api,int type) noexcept {
        if(type==10){
            if(api>=128&&api<272)return (api-128)/36;
            if(api>=709&&api<713)return api-709;
            if(api>=715&&api<811)return (api-715)/24;
        }else{
            if(api>=96&&api<192)return (api-96)/24;
            if(api>=384&&api<432)return (api-384)/12;
        }
        return -1;
    }
    static int sourceForCell(int cell) noexcept {
        if(cell>=320&&cell<656)return (cell-320)/84;
        if(cell>=1546&&cell<1550)return cell-1546;
        if(cell>=1552&&cell<1648)return (cell-1552)/24;
        if(cell>=1768&&cell<1816)return (cell-1768)/12;
        return -1;
    }
    bool alternativeSourceBusy(int source)const noexcept {
        return alternativeFlight_.valid&&(alternativeFlight_.global||(source>=0&&source<4&&alternativeFlight_.plan.from[static_cast<unsigned>(source)]!=alternativeFlight_.plan.to[static_cast<unsigned>(source)]));
    }
    bool beginGlobalAlternativeChange(const AlternativeArbiter::Plan& plan,const GlobalSnapshot& image,bool redRestore) noexcept {
        if(topPulse_.hasOwedOff()||core_.nativeNav().hasStage()||alternativeSerial_==UINT64_MAX)return false;
        AlternativeFlight flight{};flight.valid=flight.global=true;flight.redRestore=redRestore;flight.plan=plan;flight.image=image;
        flight.serial=alternativeSerial_+1;flight.presetSlot=applyingPresetSlot_;flight.reserved=2048;
        flight.acceptedOrder=applyingGlobalOrder_?applyingGlobalOrder_:musicalScheduler_.lastOrder();
        std::array<SourcePacket,144> offs{};unsigned count=0;std::array<bool,544> seen{};
        const auto affected=[&](int api) noexcept {const int source=sourceForPacket(api,11);return source>=0&&plan.from[static_cast<unsigned>(source)]!=plan.to[static_cast<unsigned>(source)];};
        const auto add=[&](int api,int type,int key=127) noexcept {
            if(count==offs.size())return false;
            offs[count++]={true,0,type,api,0,{static_cast<uint8_t>(0x86+api/128),static_cast<uint8_t>(api%128),0},key,flight.serial};
            seen[static_cast<unsigned>(api)]=true;return true;
        };
        for(int key=0;key<36;++key){const int api=core_.output().owner(key);if(api>=0&&affected(api)&&!add(api,13,key))return false;}
        for(int api=0;api<528;++api)if(affected(api)&&core_.output().debt(api)&&!seen[static_cast<unsigned>(api)]&&!add(api,11))return false;
        flight.waiting=count;alternativeSerial_=flight.serial;alternativeFlight_=flight;sourceRead_=sourceWrite_;
        for(unsigned i=0;i<count;++i){auto packet=offs[i];packet.token=++sourcePacketToken_;sourceMidi_[sourceWrite_++%sourceMidi_.size()]=packet;}
        for(auto& hold:spots_){const int source=sourceForCell(hold.cell);if(source>=0&&plan.from[static_cast<unsigned>(source)]!=plan.to[static_cast<unsigned>(source)])hold={};}
        for(auto& hold:topHold_){const int source=sourceForRoute(hold.route);if(source>=0&&plan.from[static_cast<unsigned>(source)]!=plan.to[static_cast<unsigned>(source)])hold={};}
        return count?true:finishAlternativeChange();
    }
    bool beginAlternativeChange() noexcept {
        if(alternativeFlight_.valid)return alternativeFlight_.waiting==0?finishAlternativeChange():true;
        AlternativeArbiter::Ready ready{};
        for(unsigned i=0;i<4;++i)ready[i]=alternativeBaseValid_[i];
        auto plan=alternatives_.prepareNextBar(ready);if(!plan.valid)return false;
        if(plan.from==plan.to)return true;
        if(topPulse_.hasOwedOff()||core_.nativeNav().hasStage()||alternativeSerial_==UINT64_MAX)return false;
        AlternativeFlight flight{};flight.valid=true;flight.plan=plan;flight.bases=alternativeBases_;flight.serial=alternativeSerial_+1;
        std::array<SourcePacket,144> offs{};unsigned offCount=0;std::array<bool,544> seen{};
        const auto affected=[&](int api,int type) noexcept {const int group=sourceForPacket(api,type);return group>=0&&plan.from[static_cast<unsigned>(group)]!=plan.to[static_cast<unsigned>(group)];};
        const auto addOff=[&](int api,int type,int key=127) noexcept {
            if(offCount==offs.size())return false;
            offs[offCount++]={true,0,type,api,0,{static_cast<uint8_t>(0x86+api/128),static_cast<uint8_t>(api%128),0},key,flight.serial};
            seen[static_cast<unsigned>(api)]=true;return true;
        };
        for(int key=0;key<36;++key){const int api=core_.output().owner(key);
            if(api>=0&&affected(api,11)&&!addOff(api,13,key))return false;}
        for(int api=0;api<528;++api)if(affected(api,11)&&core_.output().debt(api)&&!seen[static_cast<unsigned>(api)]&&!addOff(api,11))return false;
        std::array<SourcePacket,2048> retained{};unsigned keep=0;
        for(unsigned n=0;n<sourceWrite_-sourceRead_;++n){const auto packet=sourceMidi_[(sourceRead_+n)%sourceMidi_.size()];
            if(!affected(packet.api,packet.type))retained[keep++]=packet;}
        for(unsigned source=0;source<4;++source)if(plan.from[source]!=plan.to[source])flight.reserved+=97;
        if(keep+offCount+flight.reserved>sourceMidi_.size())return false;
        alternativeSerial_=flight.serial;flight.waiting=offCount;alternativeFlight_=flight;
        sourceRead_=sourceWrite_;
        for(unsigned n=0;n<keep;++n)sourceMidi_[sourceWrite_++%sourceMidi_.size()]=retained[n];
        for(unsigned n=0;n<offCount;++n){auto packet=offs[n];packet.token=++sourcePacketToken_;sourceMidi_[sourceWrite_++%sourceMidi_.size()]=packet;}
        for(unsigned source=0;source<4;++source)if(plan.from[source]!=plan.to[source]){
            musicalScheduler_.cancel(1,static_cast<uint16_t>(source));sourcePending_[source]=-1;cueLoad_.cancelPending(source);
            if(sourceStatus_[source]==SourceRecallStatus::Pending)sourceStatus_[source]=SourceRecallStatus::None;
            if(pairedTopSource_==static_cast<int>(source))pairedTopSource_=-1;
            for(auto& hold:spots_)if(sourceForCell(hold.cell)==static_cast<int>(source))hold={};
            for(auto& hold:topHold_)if(sourceForRoute(hold.route)==static_cast<int>(source))hold={};
        }
        return offCount?true:finishAlternativeChange();
    }
    bool finishAlternativeChange() noexcept {
        if(!alternativeFlight_.valid||alternativeFlight_.waiting)return false;
        const auto flight=alternativeFlight_;
        if(flight.global){const auto previous=applyingGlobalOrder_;applyingGlobalOrder_=flight.acceptedOrder;
            const bool applied=applyGlobal(flight.image,flight.redRestore);applyingGlobalOrder_=previous;return applied;}
        auto draft=alternatives_;
        if(!draft.finalize(flight.plan,true))return false;
        const int config=core_.nativeLive().sessionConfig();std::array<SourcePatchBank::Values,4> before{};
        std::array<int,388> cells{},values{};unsigned fields=0;
        std::array<SourcePacket,388> packets{};unsigned count=0;
        for(unsigned source=0;source<4;++source)if(flight.plan.from[source]!=flight.plan.to[source]){
            for(unsigned f=0;f<60;++f){const int cell=SourcePatchBank::cell(source,f);const auto prior=core_.nativeLive().desired(cell);
                before[source][f]=prior.known?prior.value:-2;cells[fields]=cell;values[fields++]=flight.bases[source][f];}
            for(int f=0;f<24;++f){cells[fields]=1552+24*static_cast<int>(source)+f;values[fields++]=0;}
            for(int f=0;f<12;++f){cells[fields]=1768+12*static_cast<int>(source)+f;values[fields++]=0;}
            cells[fields]=1546+static_cast<int>(source);values[fields++]=0;
            for(unsigned f=0;f<97;++f){
                int api=-1,type=10,value=0;
                if(f<60){api=SourcePatchBank::api(source,f);type=SourcePatchBank::type(f);value=flight.bases[source][f];}
                else if(f<84)api=715+24*static_cast<int>(source)+static_cast<int>(f)-60;
                else if(f<96){api=384+12*static_cast<int>(source)+static_cast<int>(f)-84;type=11;}
                else api=709+static_cast<int>(source);
                const auto status=static_cast<uint8_t>((type==10?0xB6:value?0x96:0x86)+api/128);
                packets[count++]={true,0,type,api,value,{status,static_cast<uint8_t>(api%128),static_cast<uint8_t>(type==10?value:value?127:0)}};
            }
        }
        if(sourceWrite_-sourceRead_+count>sourceMidi_.size()||!core_.commitNativeAlternativeBases(cells,values,fields,config))return false;
        alternatives_=draft;alternativeFlight_={};alternativeBaseValid_.fill(false);
        for(unsigned source=0;source<4;++source)if(flight.plan.from[source]!=flight.plan.to[source]){
            if(flight.plan.from[source]>=0)alternativeBank_.remember(source/2,static_cast<unsigned>(flight.plan.from[source]),before[source]);
            disengageSourceMemories(source);
        }
        for(unsigned n=0;n<count;++n){auto packet=packets[n];packet.token=++sourcePacketToken_;sourceMidi_[sourceWrite_++%sourceMidi_.size()]=packet;
            if(packet.type==10)pickup_.rearm(packet.api);}
        return true;
    }
    struct AlternativeFlight {bool valid{},global{},redRestore{};AlternativeArbiter::Plan plan{};std::array<SourcePatchBank::Values,4> bases{};
        GlobalSnapshot image{};std::uint64_t serial{},acceptedOrder{};unsigned waiting{},reserved{};int presetSlot{-1};};
    AlternativeArbiter alternatives_{};
    AlternativeBank alternativeBank_{};
    AlternativeFlight alternativeFlight_{};
    std::array<SourcePatchBank::Values,4> alternativeBases_{};
    std::array<bool,4> alternativeBaseValid_{};
    std::array<std::uint64_t,4> alternativePendingOrder_{};
    std::uint64_t alternativeArbitration_{},alternativeSerial_{};
    int lastInstrumentGroup_{-1},presetInstrumentGroup_{-1};
    struct AlternativeSelector {bool held{};int source{-1},slot{-1};int64_t down{};std::uint64_t navRevision{};};
    std::array<AlternativeSelector,8> alternativeSelectors_{};
    using PresetSignature=std::array<bool,19>; // IDs 1,3..20, ID2 PRESET excluded
    PresetSignature presetSignature()const noexcept {
        PresetSignature signature{};const auto& n=core_.nativeNav().active();
        const bool sample=n.contextMode==3,machine=n.contextMode==7,exposed=!sample&&!machine;
        const bool yellow=n.at(291)==1,inst=exposed&&!yellow,ylw=exposed&&yellow;
        signature[0]=n.at(292)==5;signature[2]=sample;signature[3]=yellow;
        signature[5]=inst&&n.at(299)==1;signature[6]=inst&&n.at(300)==1;signature[4]=signature[5]||signature[6];
        signature[8]=ylw&&n.at(301)==1;signature[9]=ylw&&n.at(302)==1;signature[7]=signature[8]||signature[9];
        signature[14]=inst&&n.at(303)==1;signature[15]=ylw&&n.at(304)==1;
        for(unsigned i=0;i<3;++i)if(core_.surface().held(32+i)&&topHold_[i].context==0){
            if(inst&&topHold_[i].root==0&&n.at(303)==0)signature[10]=true;
            if(ylw&&topHold_[i].root==1&&n.at(304)==0&&i<2)signature[11]=true;
        }
        for(int i=0;i<6;++i){
            const int cell=yellow?1870+i:1864+i;const auto value=core_.nativeLive().desired(cell);
            if((signature[14]||signature[15])&&value.known&&value.value==1)signature[i<3?12:13]=true;
        }
        signature[17]=machine;signature[18]=redActive_;return signature;
    }
    void invalidatePresetsIfChanged(const PresetSignature& before) noexcept {
        if(before==presetSignature())return;
        presetValid_.fill(false);pendingPresetSlot_=-1;initPending_=false;
        if(alternativeFlight_.valid&&alternativeFlight_.global&&!alternativeFlight_.redRestore){alternativeFlight_={};alternatives_.stop();}
        musicalScheduler_.cancel(3,0);musicalScheduler_.cancel(4,0);
    }
    DocumentImage captureDocument()const noexcept {
        DocumentImage d{};d.live=core_.nativeLive().snapshot();d.current=captureGlobal();
        // SOFT choices are persistent selection; sounding gates are transient.
        d.current.soft.sounding.fill(false);
        for(const auto& hold:spots_)if(hold.cell>=0&&(hold.mode==3||hold.mode==4)){
            d.live.desired[static_cast<unsigned>(hold.cell)]=static_cast<int16_t>(hold.previous);
            d.current.live.desired[static_cast<unsigned>(hold.cell)]=static_cast<int16_t>(hold.previous);
        }
        d.sample=s8Sample_;d.machine=s8Machine_;d.source=sourcePatches_;d.alternative=alternativeBank_;
        d.presets=presets_;d.init=globalInit_;
        for(int owner=0;owner<28;++owner){
            for(int slot=0;slot<(owner<12?4:3);++slot){l01::BankS6::MemoryRecord m{};
                if(core_.readNativeMemory(owner,slot,m))d.memories[static_cast<unsigned>(l01::BankS6::memoryIndex(owner,slot))]=m;}
            for(int slot=0;slot<3;++slot){l01::BankS6::PatchRecord p{};
                if(core_.readNativePatch(owner,slot,p))d.legacyPatches[static_cast<unsigned>(3*owner+slot)]=p;}
        }
        return d;
    }
    bool applyDocument(DocumentImage& d)noexcept {
        if(!validDocument(d))return false;
        l01::BankS6 bank{};bank.initializeEmpty();
        std::array<std::uint32_t,96> memoryBases{};std::array<std::uint32_t,84> patchBases{};
        for(int owner=0;owner<28;++owner){
            for(int slot=0;slot<(owner<12?4:3);++slot){const auto& m=d.memories[static_cast<unsigned>(l01::BankS6::memoryIndex(owner,slot))];
                if(m.valid){if(!bank.saveMemory(owner,slot,m.config,m.values,0))return false;memoryBases[static_cast<unsigned>(l01::BankS6::memoryIndex(owner,slot))]=m.revision-1;}}
            for(int slot=0;slot<3;++slot){const auto& p=d.legacyPatches[static_cast<unsigned>(3*owner+slot)];
                if(p.valid){if(!bank.savePatch(owner,slot,p.config,p.values,0))return false;patchBases[static_cast<unsigned>(3*owner+slot)]=p.revision-1;}}
        }
        AlternativeArbiter arbiter{};AlternativeArbiter::Ready ready{};ready.fill(true);
        if(!arbiter.finalize(arbiter.prepareRecall(d.current.alternatives,ready),true))return false;
        auto soft=SoftMidiBridge{};auto mute=muteBridge_;
        auto appliedSoft=d.current.soft;appliedSoft.sounding.fill(false);
        if(!soft.restoreApplied(appliedSoft)||!mute.restoreApplied(d.current.mute,true))return false;
        if(!core_.commitNativeDocument(d.live,d.current.nav,bank,memoryBases,patchBases))return false;
        s8Sample_=d.sample;s8Machine_=d.machine;sourcePatches_=d.source;alternativeBank_=d.alternative;alternatives_=arbiter;
        presets_=d.presets;globalInit_=d.init;soft_=soft;muteBridge_=mute;muteGate_.restoreVisible(d.current.muteVisible);
        cueLoad_=CueLoadBar{}; // desired selection is rebuilt below; no historical service pulse
        cueLoad_.restoreDocumentSelections(d.current.cueSelections);
        memoryEngaged_=d.current.memoryEngaged;presetEngaged_=d.current.presetEngaged;presetValid_.fill(false);
        musicalScheduler_.stop();sourcePending_.fill(-1);sourceStatus_.fill(SourceRecallStatus::None);
        for(auto& pending:memoryPending_)pending={};
        samplePatch_={};pairedTopSource_=-1;alternativeFlight_={};alternativeBaseValid_.fill(false);
        pendingPresetSlot_=-1;initPending_=false;redActive_=false;redHeld_=false;redImage_.valid=false;
        sourceRead_=sourceWrite_;sourceBlocked_=false;core_.output()=l01::OutputS4{};core_.enableTestMidiProjection(true);
        pickup_=ContinuousPickup{};muteLeds_.invalidate();muteMidi_.invalidate();muteMidi_.observe(globalMute());
        for(unsigned cell=0;cell<d.live.desired.size();++cell){if(!GlobalSnapshot::inScope(cell))continue;int type=10;const int api=globalApi(cell,type);int value=d.live.desired[cell];
            if(cell>=1337&&cell<=1339){const int m=d.live.desired[1341];if(value<0||m<0)continue;value=(value*m+63)/127;}
            if(api<0||value<0)continue;
            SourcePacket packet{};packet.valid=true;packet.token=++sourcePacketToken_;packet.type=type;packet.api=api;packet.value=value;
            packet.bytes={static_cast<uint8_t>((type==10?0xB6:value?0x96:0x86)+api/128),static_cast<uint8_t>(api%128),static_cast<uint8_t>(type==10?value:value?127:0)};
            sourceMidi_[sourceWrite_++%sourceMidi_.size()]=packet;
        }
        return true;
    }
    GlobalSnapshot captureGlobal()const noexcept {
        GlobalSnapshot image{};image.live=core_.nativeLive().snapshot();image.nav=core_.nativeNav().active();
        for(unsigned cell=0;cell<image.live.desired.size();++cell)
            if(!GlobalSnapshot::inScope(cell))image.live.desired[cell]=-2;
        image.mute=muteBridge_.state().applied();image.muteVisible=mutePageVisible();
        image.alternatives=alternatives_.active();image.soft=soft_.applied();image.cueSelections=cueLoad_.desiredSelections();image.memoryEngaged=memoryEngaged_;image.presetEngaged=presetEngaged_;
        image.valid=true;return image;
    }
    static int globalApi(unsigned cell,int& type) noexcept {
        type=10;
        if(cell<320){if(cell%40<16)return 16*static_cast<int>(cell/40)+static_cast<int>(cell%40);
            type=11;return 12*static_cast<int>(cell/40)+static_cast<int>(cell%40)-16;}
        if(cell<1328){const int owner=static_cast<int>((cell-320)/42),field=static_cast<int>((cell-320)%42);
            if(field<18)return 128+18*owner+field;
            type=11;return field<30?96+12*owner+field-18:-1;}
        if(cell>=1328&&cell<1477)return cell==1340||cell==1341?-1:static_cast<int>(cell)-768;
        if(cell>=1546&&cell<1768)return static_cast<int>(cell)-837;
        type=11;
        if(cell>=1477&&cell<1513)return static_cast<int>(cell)-985;
        if(cell>=1768&&cell<1864)return static_cast<int>(cell)-1384;
        if(cell>=1864&&cell<1876)return static_cast<int>(cell)-1384;
        return -1;
    }
    bool applyGlobal(const GlobalSnapshot& image,bool redRestore) noexcept {
        const auto& live=core_.nativeLive();
        if(!image.admissible(live.sessionConfig())||live.revision()>=l01::LiveState::MaxRevision||globalApplySerial_==UINT64_MAX)return false;
        AlternativeArbiter::Ready ready{};
        for(unsigned source=0;source<4;++source){SourcePatchBank::Values ignored{};
            ready[source]=image.alternatives[source]<0||alternativeBank_.base(source/2,static_cast<unsigned>(image.alternatives[source]),live.sessionConfig(),ignored);}
        const auto plan=alternatives_.prepareRecall(image.alternatives,ready);if(!plan.valid)return false;
        bool closing=false;
        for(int api=0;api<528;++api){const int source=sourceForPacket(api,11);
            if(source>=0&&plan.from[static_cast<unsigned>(source)]!=plan.to[static_cast<unsigned>(source)]&&core_.output().debt(api))closing=true;}
        for(int key=0;key<36;++key){const int api=core_.output().owner(key),source=sourceForPacket(api,11);
            if(api>=0&&source>=0&&plan.from[static_cast<unsigned>(source)]!=plan.to[static_cast<unsigned>(source)])closing=true;}
        if(closing)return beginGlobalAlternativeChange(plan,image,redRestore);
        const auto cutoff=applyingGlobalOrder_?applyingGlobalOrder_:musicalScheduler_.lastOrder();
        AlternativeArbiter::Ready preserve{};
        for(unsigned source=0;source<4;++source)preserve[source]=alternatives_.pending()[source]>=0&&alternativePendingOrder_[source]>cutoff;
        auto alternativeDraft=alternatives_;if(!alternativeDraft.finalize(plan,true,preserve))return false;
        const auto before=live.snapshot();auto target=before;target.revision=live.revision()+1;
        for(unsigned cell=0;cell<target.desired.size();++cell)
            if(GlobalSnapshot::inScope(cell))target.desired[cell]=image.live.desired[cell];
        auto mute=muteBridge_;auto soft=soft_;auto cue=cueLoad_;
        if(!mute.restoreApplied(image.mute,redRestore)||!soft.restoreApplied(image.soft)||!cue.restoreSelections(image.cueSelections))return false;
        std::array<SourcePacket,2048> packets{};unsigned count=0;std::array<bool,544> off{};
        const auto append=[&](int api,int type,int value,int key=127) noexcept {
            if(count==packets.size()||api<0)return false;
            const bool on=type==11&&value==1;
            const auto status=static_cast<uint8_t>((type==10?0xB6:on?0x96:0x86)+api/128);
            packets[count++]={true,0,type,api,value,{status,static_cast<uint8_t>(api%128),static_cast<uint8_t>(type==10?value:on?127:0)},key};
            return true;
        };
        // Close captured TOUCH owners first. A failing OFF blocks later ONs.
        for(int key=0;key<36;++key){const int api=core_.output().owner(key);
            if(api>=0){if(!append(api,13,0,key))return false;if(api<544)off[static_cast<unsigned>(api)]=true;}}
        for(unsigned api=0;api<528;++api)if(core_.output().debt(static_cast<int>(api))&&!off[api]){
            if(!append(static_cast<int>(api),11,0))return false;
            off[api]=true;
        }
        for(unsigned cell=0;cell<target.desired.size();++cell){
            if(!GlobalSnapshot::inScope(cell))continue;
            int type=10;const int api=globalApi(cell,type);if(api<0)continue;
            int value=target.desired[cell];
            // LOOP m is internal. Only its three effective gain destinations emit.
            if(cell>=1337&&cell<=1339){const int m=target.desired[1341];if(value<0||m<0)continue;value=(value*m+63)/127;}
            if(type==11){
                if(value<0){if(!core_.output().debt(api)||off[static_cast<unsigned>(api)])continue;value=0;}
                if(value==0&&off[static_cast<unsigned>(api)])continue;
            }else if(value<0)continue;
            if(!append(api,type,value))return false;
        }
        const bool wasPresetFrame=core_.nativeNav().active().at(292)==1&&core_.nativeNav().active().presetFlag==1&&!mutePageVisible();
        if(count>sourceMidi_.size()||!core_.commitNativeGlobalRecall(target,image.nav))return false;
        for(unsigned source=0;source<4;++source)if(plan.from[source]>=0&&plan.from[source]!=plan.to[source]){
            SourcePatchBank::Values last{};for(unsigned f=0;f<60;++f)last[f]=before.desired[static_cast<unsigned>(SourcePatchBank::cell(source,f))];
            alternativeBank_.remember(source/2,static_cast<unsigned>(plan.from[source]),last);
        }
        for(unsigned source=0;source<4;++source)if(plan.from[source]!=plan.to[source]){
            musicalScheduler_.cancel(1,static_cast<uint16_t>(source));sourcePending_[source]=-1;cue.cancelUnstarted(source);
            disengageSourceMemories(source);
            for(auto& hold:topHold_)if(sourceForRoute(hold.route)==static_cast<int>(source))hold={};
        }
        alternatives_=alternativeDraft;alternativeFlight_={};bool pendingAlternative=false;
        for(unsigned source=0;source<4;++source){if(alternatives_.pending()[source]<0)alternativeBaseValid_[source]=false;else pendingAlternative=true;}
        if(!pendingAlternative)musicalScheduler_.cancel(5,0);
        // Whole-world recall supersedes older queued absolute values, while
        // owed OFFs above survive and future musical requests remain queued.
        // Keep sourceBlocked_: a failed OFF still waits until next render.
        sourceRead_=sourceWrite_;
        for(unsigned i=0;i<count;++i){auto packet=packets[i];packet.token=++sourcePacketToken_;sourceMidi_[sourceWrite_++%sourceMidi_.size()]=packet;}
        for(unsigned cell=0;cell<target.desired.size();++cell){if(!GlobalSnapshot::inScope(cell))continue;
            int type=10;const int api=globalApi(cell,type);
            if(type==10&&api>=0&&before.desired[cell]!=target.desired[cell])pickup_.rearm(api);
        }
        muteBridge_=mute;soft_=soft;cueLoad_=cue;muteGate_.restoreVisible(image.muteVisible);muteMidi_.observe(globalMute());
        if(!wasPresetFrame&&image.nav.at(292)==1&&image.nav.presetFlag==1&&!image.muteVisible)presetInstrumentGroup_=lastInstrumentGroup_;
        memoryEngaged_=image.memoryEngaged;presetEngaged_=image.presetEngaged;
        for(unsigned owner=0;owner<28;++owner)for(int slot=0;slot<(owner<12?4:3);++slot){
            l01::BankS6::MemoryRecord record{};
            if(!core_.readNativeMemory(static_cast<int>(owner),slot,record)||!record.valid||record.config!=target.config)
                memoryEngaged_[static_cast<unsigned>(l01::BankS6::memoryIndex(static_cast<int>(owner),slot))]=false;
        }
        for(auto& hold:spots_)hold={};
        ++globalApplySerial_;return true;
    }
    std::array<GlobalSnapshot,4> presets_{};
    GlobalSnapshot pendingPresetImage_{},redImage_{},globalInit_{};
    std::array<bool,4> presetEngaged_{},presetValid_{};
    int pendingPresetSlot_{-1},applyingPresetSlot_{-1};
    bool redActive_{},redHeld_{},redWasActive_{},redDirectGesture_{},initPending_{};int64_t redDownMs_{};
    std::uint64_t globalApplySerial_{},applyingGlobalOrder_{};
    struct MemoryPending {int slot{-1},config{-2};l01::BankS6::MemoryImage values{};};
    bool memoryGesture(int owner,int slot) noexcept {
        return memoryEngaged(owner,slot)?requestMemoryRecall(owner,slot):captureMemory(owner,slot);
    }
    void disengageSourceMemories(unsigned source) noexcept {
        // Only the three explicitly mapped INST/YELLOW pages of this source.
        for(unsigned page=3*source;page<3*source+3;++page){
            musicalScheduler_.cancel(2,static_cast<uint16_t>(page));memoryPending_[page]={};
            for(int slot=0;slot<4;++slot)memoryEngaged_[static_cast<unsigned>(l01::BankS6::memoryIndex(static_cast<int>(page),slot))]=false;
        }
    }
    void applyMemoryRecall(unsigned owner) noexcept {
        if(owner>=28)return;
        auto request=memoryPending_[owner];memoryPending_[owner]={};if(request.slot<0)return;
        std::array<int,97> cells{},values{};std::array<bool,9> changed{};
        for(unsigned f=0;f<9;++f){cells[f]=l01::FxS6::addressFor(static_cast<int>(owner),static_cast<int>(f));values[f]=request.values[f];
            auto prior=core_.nativeLive().desired(cells[f]);changed[f]=!prior.known||prior.value!=values[f];
            for(const auto& hold:spots_)if(hold.cell==cells[f])return;
        }
        if(sourceWrite_-sourceRead_+9+alternativeFlight_.reserved>sourceMidi_.size()||!core_.commitNativeSourceRecall(cells,values,9,request.config))return;
        for(unsigned f=0;f<9;++f){
            int api=cells[f]-(f<6?837:1384),type=f<6?10:11,value=values[f];
            if(type==10&&changed[f])pickup_.rearm(api);
            auto status=static_cast<uint8_t>((type==10?0xB6:value?0x96:0x86)+api/128);
            sourceMidi_[sourceWrite_++%sourceMidi_.size()]={true,++sourcePacketToken_,type,api,value,
                {status,static_cast<uint8_t>(api%128),static_cast<uint8_t>(type==10?value:value?127:0)}};
        }
    }
    bool transportStopped_{}; // false also means unknown: never guess a stopped clock
    HostActionScheduler musicalScheduler_{};
    std::array<MemoryPending,28> memoryPending_{};
    std::array<bool,96> memoryEngaged_{};
    void applySourceRecall(unsigned source) noexcept {
        if(source>=4)return;
        if(pairedTopSource_==static_cast<int>(source))pairedTopSource_=-1;
        {
            const int slot=sourcePending_[source];if(slot<0)return;
            sourcePending_[source]=-1;
            if(sourcePendingAlternative_[source]!=activeAlternative(source)){sourceStatus_[source]=SourceRecallStatus::Busy;return;}
            const int config=core_.nativeLive().sessionConfig();
            if(config<1){sourceStatus_[source]=SourceRecallStatus::UnknownConfig;return;}
            SourcePatchBank::Values patch{};patch.fill(0);
            const int instrument=activeAlternative(source);
            if(slot==3&&instrument>=0&&!alternativeBank_.base(source/2,static_cast<unsigned>(instrument),config,patch)){
                sourceStatus_[source]=SourceRecallStatus::UnknownConfig;return;}
            if(slot<3){SourcePatchBank::Patch saved{};
                if(!sourcePatches_.read(source,static_cast<unsigned>(slot),saved)||saved.config!=config){
                    sourceStatus_[source]=SourceRecallStatus::EmptyPatch;return;}
                patch=saved.values;
            }
            std::array<int,97> cells{},values{};
            unsigned count=97;
            std::array<int,97> apis{},types{};
            std::array<bool,97> changed{};
            for(unsigned f=0;f<60;++f){
                cells[f]=SourcePatchBank::cell(source,f);values[f]=patch[f];
                apis[f]=SourcePatchBank::api(source,f);types[f]=SourcePatchBank::type(f);
            }
            for(unsigned f=0;f<24;++f){cells[60+f]=1552+24*static_cast<int>(source)+static_cast<int>(f);apis[60+f]=cells[60+f]-837;types[60+f]=10;}
            for(unsigned f=0;f<12;++f){cells[84+f]=1768+12*static_cast<int>(source)+static_cast<int>(f);apis[84+f]=cells[84+f]-1384;types[84+f]=11;}
            cells[96]=1546+static_cast<int>(source);apis[96]=cells[96]-837;types[96]=10;
            bool busy=false;
            for(unsigned f=0;f<count;++f){
                const auto prior=core_.nativeLive().desired(cells[f]);
                changed[f]=!prior.known||prior.value!=values[f];
                for(const auto& hold:spots_)if(hold.cell==cells[f])busy=true;
            }
            if(busy||sourceWrite_-sourceRead_+count+alternativeFlight_.reserved>sourceMidi_.size()||
               !core_.commitNativeSourceRecall(cells,values,count,config)){
                sourceStatus_[source]=SourceRecallStatus::Busy;return;
            }
            for(unsigned f=0;f<count;++f){
                int api=apis[f],type=types[f],value=values[f];
                if(type==10&&changed[f])pickup_.rearm(api);
                const auto status=static_cast<uint8_t>((type==10?0xB6:value?0x96:0x86)+api/128);
                auto& p=sourceMidi_[sourceWrite_++%sourceMidi_.size()];
                p={true,++sourcePacketToken_,type,api,value,{status,static_cast<uint8_t>(api%128),static_cast<uint8_t>(type==10?value:value?127:0)}};
            }
            disengageSourceMemories(source);
            sourceStatus_[source]=SourceRecallStatus::AppliedLocal;
        }
    }
    SourcePatchBank sourcePatches_{};
    std::array<int,4> sourcePending_{{-1,-1,-1,-1}},sourcePendingAlternative_{{-1,-1,-1,-1}};
    std::array<SourceRecallStatus,4> sourceStatus_{};
    std::array<SourcePacket,2048> sourceMidi_{};
    unsigned sourceRead_{},sourceWrite_{},sourcePacketToken_{};
    bool sourceBlocked_{};
    CueLoadBar cueLoad_{};
    std::array<bool,4> topSaveSuppress_{};
    unsigned sampleSavesAccepted_{},sampleSavesRefused_{};
    std::array<std::uint32_t,36> groupDownEpoch_{};
    std::uint32_t groupEpochCounter_{1};
    DjBrowserWindow browser_{};
    std::array<TopCapture,4> topHold_{};
    std::array<TopIntent,4> topIntents_{};
    std::array<bool,4> topIntentValid_{};
    unsigned topResetsDeferred_{},topPatchRecallsDeferred_{};
    BrowserLoadRequest browserLoadRequest_{};
    uint32_t browserSelectionSequence_{};bool browserLoadPending_{};
    uint32_t browserLoadIntents_{};int lastBrowserLoadDeck_{};
    uint32_t dj2QualifiedTurns_{};int dj2LastQualifiedDelta_{};
    bool dj2PushRotCaptured_{},dj2LoopStartRequested_{};
    Dj2LoopObserved dj2LoopObserved_{Dj2LoopObserved::Unknown};
    uint32_t dj2LoopRequests_{},dj2LoopStopRequests_{},dj2BackendGeneration_{};
    std::array<SpotHold,12> spots_{};
    std::array<int,702> stopAddresses_{};
    std::array<bool,702> stopAwaiting_{};
    unsigned stopCount_{},stopRead_{},stopPendingCount_{};int64_t stopSample_{};
    std::atomic<int> presetFaceRequest_{-1};
    std::atomic<bool> publishedPresetFace_{false};
    bool started_{};
    bool historicalL02Bench_{};
    std::unique_ptr<DocumentExchange> documents_{std::make_unique<DocumentExchange>()};
    std::atomic<uint32_t> restoreEpoch_{0};bool documentDirty_{true};
    SessionMailbox restores_{},snapshots_{};
    SessionFrame uiSession_{}; // UI owns cache and request generation.
    uint32_t requestedToken_{}; // UI only
    uint32_t appliedToken_{},publishedToken_{},publishedRevision_{}; // render only
    std::array<Trace,TraceCapacity> trace_{};
    std::atomic<uint32_t> read_{0},write_{0},dropped_{0},overflows_{0};
    uint32_t sequence_{};
    uint32_t confirmedOffSequence_{};int confirmedOffApi_{-1};
};
}
