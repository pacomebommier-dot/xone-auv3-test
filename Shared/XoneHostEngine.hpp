#pragma once
// L02 host slice only. Musical S3/S8 paths remain unarmed.
#include "../Core/NativeSlices.hpp"
#include <atomic>
#include <array>
#include <cstdint>
#include <cstddef>
namespace xone::host {
enum class Stage : uint8_t { RxIn, Decision, TxOut, RxFeedback, Stop, Overflow };
enum class Reason : uint8_t { Mapped, SurfaceOnly, Rejected, Untrusted, Sent, SendFailed, NoOutput };
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
};
class HostEngine {
public:
    static constexpr uint32_t TraceCapacity=256;
    static_assert(std::atomic<uint32_t>::is_always_lock_free,"RT trace requires lock-free indices");
    // Called only by render thread. Host cable is NOT an authenticated device ID.
    Result receive(uint8_t cable,const uint8_t* data,size_t length,int64_t sample) noexcept {
        const auto seq=++sequence_;
        record(seq,sample,cable?Stage::RxFeedback:Stage::RxIn,Reason::Rejected,cable,data,length);
        if(cable) {record(seq,sample,Stage::Decision,Reason::Untrusted,cable,data,length);return {};}
        if(!data || length!=3 || data[1]>127 || data[2]>127) {
            record(seq,sample,Stage::Decision,Reason::Rejected,cable,data,length);return {};
        }
        // Reuse archive L01/11 SurfaceInput verbatim, diagnostic only on MIDI15.
        if((data[0]&15)==14) {
            const auto s=surface_.accept({14,data[0]&0xF0,data[1],data[2]});
            record(seq,sample,Stage::Decision,s.kind==l01::SurfaceKind::None?Reason::Rejected:Reason::SurfaceOnly,cable,data,length);
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
    void sent(const Result& r,int32_t status) noexcept {
        record(r.sequence,r.sample,Stage::TxOut,status==0?Reason::Sent:Reason::SendFailed,0,r.bytes,3,status);
    }
    void noOutput(const Result& r) noexcept {
        record(r.sequence,r.sample,Stage::Decision,Reason::NoOutput,0,r.bytes,3);
    }
    void stop(int64_t sample) noexcept {
        surface_.onHostStop(); // must not fabricate UP or clear held keys
        record(++sequence_,sample,Stage::Stop,Reason::SurfaceOnly,0,nullptr,0);
    }
    int observed() const noexcept {return -2;} // no authenticated backend profile in L02
    bool surfaceHeld(size_t i) const noexcept {return surface_.held(i);}
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
    l01::SurfaceInput surface_{};
    std::array<Trace,TraceCapacity> trace_{};
    std::atomic<uint32_t> read_{0},write_{0},dropped_{0},overflows_{0};
    uint32_t sequence_{};
};
}
