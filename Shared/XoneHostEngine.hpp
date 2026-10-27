#pragma once
// Native V3: K2/gestures/NAV, known-session CC/LOOP and SMART/MIX/TOUCH/FLASH; specialized S8 remains pending.
#include "../CoreV3/CoreCoordinatorV3.hpp"
#include "XoneContinuousInput.hpp"
#include "XoneSession.hpp"
#include "../Core/MutePhysicalGate.hpp"
#include <atomic>
#include <array>
#include <cstdint>
#include <cstddef>
namespace xone::host {
enum class Stage : uint8_t { RxIn, Decision, TxOut, RxFeedback, Stop, Overflow, Gesture };
enum class Reason : uint8_t { Mapped, SurfaceOnly, Rejected, Untrusted, Sent, SendFailed, NoOutput, Received, Integrated, Qualified, ClockRejected, UnknownLive, PickupWait, Musical };
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
    std::array<PendingPacket,2> extra{};
    unsigned extraCount{};
    int key{127};bool requiresOff{};
    unsigned packetCount() const noexcept {return emit?1+extraCount:0;}
    Result packet(unsigned i) const noexcept {
        if(!emit||i>extraCount)return {};
        Result r{};
        if(i==0){r={true,{bytes[0],bytes[1],bytes[2]},sequence,sample,musical,api,value,type};r.key=key;r.requiresOff=requiresOff;}
        else{const auto& p=extra[i-1];r={true,{p.bytes[0],p.bytes[1],p.bytes[2]},sequence,sample,true,p.api,p.value,p.type};r.key=p.key;r.requiresOff=p.requiresOff;}
        return r;
    }
};
class HostEngine {
public:
    HostEngine() noexcept {
        core_.initializeSourceDefaultNav();
        core_.enableDiagnosticLive(true);
        core_.enableTestMidiProjection(true);
        core_.enableDiagnosticTouchNav(true); // source @zk/@zl selection, not SPOT TOUCH output
        core_.enableDiagnosticQualifiedNav(true); // source-qualified SHORT/LONG NAV only
        core_.enableNativeMuteBot2(true); // exclusive solo BOT2 owner
    }
    bool mutePageVisible() const noexcept {return muteGate_.visible();}
    bool muteGesturePending() const noexcept {return muteBridge_.state().pendingCount()!=0;}
    const GlobalMuteState& globalMute() const noexcept {return muteBridge_.state();}
    unsigned onMusicalGrid(const GridBatch& b) noexcept {return muteBridge_.observe(b);}
    static constexpr uint32_t TraceCapacity=256;
    static_assert(std::atomic<uint32_t>::is_always_lock_free,"RT trace requires lock-free indices");
    // Called only by render thread. Host cable is NOT an authenticated device ID.
    Result receive(uint8_t cable,const uint8_t* data,size_t length,int64_t sample,int64_t gestureMs=0) noexcept {
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
            const bool accepted=core_.onMidi({14,cmd,data[1],data[2]},gestureMs);
            record(seq,sample,Stage::Decision,accepted?Reason::Integrated:Reason::ClockRejected,cable,data,length);
            drainCore(seq,sample);
            if(accepted&&(cmd==0x90||cmd==0x80)&&physical>=0){
                const bool down=cmd==0x90&&data[2]!=0;
                if(physical==1){
                    if(down)muteGate_.beginBot2(gestureMs,core_.s3().physicalOwner(1)==1);
                    else muteGate_.releaseBot2(gestureMs);
                }else if(physical>=4&&physical<20){
                    const unsigned base=static_cast<unsigned>(physical-4);
                    if(down)muteGate_.beginBase(base,gestureMs,true);
                    else muteGate_.releaseBase(base,gestureMs,muteBridge_);
                }
                // Any second BOT starts an S2 combo or concurrent BOT gesture.
                // In that case BOT2 is no longer entitled to a solo MUTE short/long.
                if(down&&physical<4&&physical!=1&&muteGate_.botHeld())muteGate_.cancelBot2();
            }
            if(accepted&&(cmd==0x90||cmd==0x80)&&physical>=20&&physical<32){
                const bool down=cmd==0x90&&data[2]!=0;
                // S2 advances before input: a LONG in this packet must qualify NAV first.
                const auto captured=core_.nativeNav().active().route(physical);
                if(wasHeld!=down)return spot(physical,down,captured,gestureMs,seq,sample);
            }
            if(accepted&&cmd==0xB0&&data[1]<20)return continuous(data[1],data[2],gestureMs,seq,sample);
            return {};
        }
        // Explicit T01 bench profile: CH1 only, exactly two CC mappings, no Thru.
        if(data[0]!=0xB0 || (data[1]!=4 && data[1]!=8)) {
            record(seq,sample,Stage::Decision,Reason::Rejected,cable,data,length);return {};
        }
        Result r{true,{0xB0,static_cast<uint8_t>(data[1]==4?74:71),data[2]},seq,sample};
        record(seq,sample,Stage::Decision,Reason::Mapped,cable,r.bytes,3);
        return r;
    }
    // Lifecycle-only restore: call before rendering starts, never from UI during render.
    bool restoreSessionBeforeRendering(const l01::LiveState::SessionImage& image) noexcept {
        return !started_&&core_.restoreNativeSessionForDiagnostic(image);
    }
    // UI/host thread only, serialized externally. Do not call core from these methods.
    bool requestSessionRestore(const SessionImage& image) noexcept {
        if(!validSession(image)||requestedToken_==0xFFFFFFFFu)return false;
        uiSession_={image,++requestedToken_};restores_.publish(uiSession_);return true;
    }
    SessionImage sessionForHost() noexcept {
        SessionFrame next{};
        if(snapshots_.take(next)&&next.token>=requestedToken_)uiSession_=next;
        return uiSession_.image;
    }
    // Render boundaries are the sole application/publication points.
    void beginRenderBuffer() noexcept {
        started_=true;
        if(!restores_.hasUpdate()||core_.output().outstanding()!=0)return;
        for(std::size_t i=0;i<36;++i)if(core_.surface().held(i))return;
        SessionFrame next{};
        if(restores_.take(next)&&core_.restoreNativeSessionForDiagnostic(next.image)){
            appliedToken_=next.token;pickup_=ContinuousPickup{};
            core_.output()=l01::OutputS4{};core_.enableTestMidiProjection(true);
        }
    }
    void endRenderBuffer() noexcept {
        const auto revision=core_.nativeLive().revision();
        if(revision==publishedRevision_&&appliedToken_==publishedToken_)return;
        snapshots_.publish({core_.nativeLive().snapshot(),appliedToken_});
        publishedRevision_=revision;publishedToken_=appliedToken_;
    }
    l01::LiveRead desiredLive(int cell) const noexcept {return core_.nativeLive().desired(cell);}
    // Called immediately before each callback; dependent FLASH ON is suppressed
    // unless its nominal OFF succeeded. Its own OFF still follows a failed ON.
    bool canSend(const Result& r) const noexcept {
        if(r.musical&&((r.type==11&&r.value==1)||r.type==12||r.type==14)&&r.sequence!=sequence_)return false;
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
        }
        record(r.sequence,r.sample,Stage::TxOut,status==0?Reason::Sent:Reason::SendFailed,0,r.bytes,3,status);
    }
    void noOutput(const Result& r) noexcept {
        if(r.type==13&&core_.output().owner(r.key)==r.api){
            core_.output().apply({11,r.api,0,127});core_.output().drain();
        }
        record(r.sequence,r.sample,Stage::Decision,Reason::NoOutput,0,r.bytes,3);
    }
    void stop(int64_t sample) noexcept {
        started_=true;
        // Keep output debts until the real callback confirms each OFF.
        const auto output=core_.output();
        stopCount_=stopRead_=0;stopSample_=sample;
        for(int a=0;a<702;++a)if(output.debt(a))stopAddresses_[stopCount_++]=a;
        muteGate_.stop();muteBridge_.stop();
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
        if(stopRead_>=stopCount_)return {};
        const int a=stopAddresses_[stopRead_++];
        Result r{true,{static_cast<uint8_t>(0x86+a/128),static_cast<uint8_t>(a%128),0},sequence_,stopSample_,true,a,0,11};
        for(int key=0;key<36;++key)if(core_.output().owner(key)==a){r.type=13;r.key=key;break;}
        return r;
    }
    int observed() const noexcept {return -2;} // no authenticated backend profile in L02
    bool surfaceHeld(size_t i) const noexcept {return core_.surface().held(i);}
    // Render-thread-only inspection; UI reads the SPSC trace, never core state.
    int navMode() const noexcept {return core_.nativeNav().active().at(292);}
    bool longNavPending() const noexcept {return core_.longNavPending();}
    void advance(int64_t ms,int64_t sample) noexcept {
        started_=true;
        muteGate_.advance(ms);
        if(!core_.gesture().timerNeeded())return;
        const auto seq=++sequence_;
        if(!core_.onTimer(ms))record(seq,sample,Stage::Decision,Reason::ClockRejected,0,nullptr,0);
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
    l01::MutePhysicalGate muteGate_{};
    MuteBarBridge muteBridge_{};
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
            const auto selected=preset>=0?core_.nativeLive().desired(preset):l01::LiveRead{true,1};
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
        if(!live.known){record(seq,sample,Stage::Decision,Reason::UnknownLive,0,nullptr,0);return {};}
        if(core_.nativeLive().revision()>=l01::LiveState::MaxRevision)return {};
        const int value=pickup_.consider(cc,target.api,live.value,raw,ms);
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
    std::array<SpotHold,12> spots_{};
    std::array<int,702> stopAddresses_{};
    unsigned stopCount_{},stopRead_{};int64_t stopSample_{};
    bool started_{};
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
