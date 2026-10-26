#pragma once
// Native V3 host: K2 surface/gestures/NAV connected; known-session continuous LIVE/S4 connected; S8 specialization remains pending.
#include "../Core/CoreCoordinator.hpp"
#include "XoneContinuousInput.hpp"
#include "XoneSession.hpp"
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
struct Result {
    bool emit{};
    uint8_t bytes[3]{};
    uint32_t sequence{};
    int64_t sample{};
    bool musical{};
    int api{-1},value{};
};
class HostEngine {
public:
    HostEngine() noexcept {
        core_.initializeSourceDefaultNav();
        core_.enableDiagnosticLive(true);
        core_.enableTestMidiProjection(true);
        core_.enableDiagnosticQualifiedNav(true); // source-qualified SHORT/LONG NAV only
    }
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
            const bool accepted=core_.onMidi({14,cmd,data[1],data[2]},gestureMs);
            record(seq,sample,Stage::Decision,accepted?Reason::Integrated:Reason::ClockRejected,cable,data,length);
            drainCore(seq,sample);
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
    void sent(const Result& r,int32_t status) noexcept {
        if(r.musical&&status==0){core_.output().apply({10,r.api,r.value,127});core_.output().drain();}
        record(r.sequence,r.sample,Stage::TxOut,status==0?Reason::Sent:Reason::SendFailed,0,r.bytes,3,status);
    }
    void noOutput(const Result& r) noexcept {
        record(r.sequence,r.sample,Stage::Decision,Reason::NoOutput,0,r.bytes,3);
    }
    void stop(int64_t sample) noexcept {
        started_=true;
        core_.onHostStop(); // must not fabricate UP or clear held keys
        record(++sequence_,sample,Stage::Stop,Reason::SurfaceOnly,0,nullptr,0);
    }
    int observed() const noexcept {return -2;} // no authenticated backend profile in L02
    bool surfaceHeld(size_t i) const noexcept {return core_.surface().held(i);}
    // Render-thread-only inspection; UI reads the SPSC trace, never core state.
    int navMode() const noexcept {return core_.nativeNav().active().at(292);}
    bool longNavPending() const noexcept {return core_.longNavPending();}
    void advance(int64_t ms,int64_t sample) noexcept {
        started_=true;
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
    Result continuous(int cc,int raw,int64_t ms,uint32_t seq,int64_t sample) noexcept {
        const auto target=continuousTarget(core_.nativeNav().active().route(36+cc));
        // LOOP needs coherent derived r1/r2/r3 batches, never raw b1/b2/b3/m.
        if(!target.valid()||(target.api>=569&&target.api<=573))return {};
        const auto live=core_.nativeLive().desired(target.cell);
        if(!live.known){record(seq,sample,Stage::Decision,Reason::UnknownLive,0,nullptr,0);return {};}
        if(core_.nativeLive().revision()>=l01::LiveState::MaxRevision)return {};
        const int value=pickup_.consider(cc,target.api,live.value,raw,ms);
        if(value<0){record(seq,sample,Stage::Decision,Reason::PickupWait,0,nullptr,0);return {};}
        auto projection=core_.output();
        if(!projection.apply({10,target.api,value,127}))return {};
        if(!core_.commitNativeLive(target.cell,value,core_.nativeLive().revision()))return {};
        if(projection.emitted()==0)return {};
        const auto packet=projection.packet(0);
        Result r{true,{packet.status,packet.number,packet.value},seq,sample,true,target.api,value};
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
    l01::CoreCoordinator core_{};
    ContinuousPickup pickup_{};
    bool started_{};
    SessionMailbox restores_{},snapshots_{};
    SessionFrame uiSession_{}; // UI owns cache and request generation.
    uint32_t requestedToken_{}; // UI only
    uint32_t appliedToken_{},publishedToken_{},publishedRevision_{}; // render only
    std::array<Trace,TraceCapacity> trace_{};
    std::atomic<uint32_t> read_{0},write_{0},dropped_{0},overflows_{0};
    uint32_t sequence_{};
};
}
