#pragma once
// Session LIVE only: versioned document bytes plus SPSC ownership hand-off.
// No PATCH/BASE, backend observations, MIDI, locks, allocation or retry protocol.
#include "../Core/LiveState.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
namespace xone::host {
using SessionImage=l01::LiveState::SessionImage;
inline bool validSession(const SessionImage& s) noexcept {
    if(s.schema!=1||s.config<1||s.config>127||s.revision<1||s.revision>l01::LiveState::MaxRevision)return false;
    for(auto v:s.desired)if(v!=-2&&(v<0||v>127))return false;
    return true;
}
constexpr std::size_t SessionByteCount=16+2*l01::LiveState::CellCount+4;
using SessionBytes=std::array<uint8_t,SessionByteCount>;
inline uint32_t sessionCrc(const uint8_t* p,std::size_t n) noexcept {
    uint32_t crc=0xFFFFFFFFu;
    for(std::size_t i=0;i<n;++i){crc^=p[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1u)?0xEDB88320u:0u);}
    return ~crc;
}
inline void sessionPut32(uint8_t* p,uint32_t v) noexcept {for(int i=0;i<4;++i)p[i]=static_cast<uint8_t>(v>>(8*i));}
inline uint32_t sessionGet32(const uint8_t* p) noexcept {uint32_t v=0;for(int i=0;i<4;++i)v|=static_cast<uint32_t>(p[i])<<(8*i);return v;}
inline bool encodeSession(const SessionImage& s,SessionBytes& out) noexcept {
    if(!validSession(s))return false;
    out.fill(0);out[0]='X';out[1]='L';out[2]='I';out[3]='V';out[4]=1;out[5]=static_cast<uint8_t>(s.config);
    sessionPut32(out.data()+8,s.revision);sessionPut32(out.data()+12,static_cast<uint32_t>(s.desired.size()));
    for(std::size_t i=0;i<s.desired.size();++i){const auto v=static_cast<uint16_t>(s.desired[i]);out[16+2*i]=static_cast<uint8_t>(v);out[17+2*i]=static_cast<uint8_t>(v>>8);}
    sessionPut32(out.data()+out.size()-4,sessionCrc(out.data(),out.size()-4));return true;
}
inline bool decodeSession(const uint8_t* bytes,std::size_t size,SessionImage& out) noexcept {
    if(!bytes||size!=SessionByteCount||bytes[0]!='X'||bytes[1]!='L'||bytes[2]!='I'||bytes[3]!='V'||
       bytes[4]!=1||bytes[6]!=0||bytes[7]!=0||sessionGet32(bytes+12)!=l01::LiveState::CellCount||
       sessionGet32(bytes+size-4)!=sessionCrc(bytes,size-4))return false;
    SessionImage s{};s.schema=1;s.config=bytes[5];s.revision=sessionGet32(bytes+8);
    for(std::size_t i=0;i<s.desired.size();++i){const unsigned v=static_cast<unsigned>(bytes[16+2*i])+(static_cast<unsigned>(bytes[17+2*i])<<8);
        if(v!=65534u&&v>127u)return false;
        s.desired[i]=v==65534u?static_cast<int16_t>(-2):static_cast<int16_t>(v);
    }
    if(!validSession(s))return false;
    out=s;return true;
}
struct SessionFrame {SessionImage image{};uint32_t token{};};
// Three slots ensure producer never writes a slot currently copied by consumer.
// Exactly one producer and one consumer. UI callers are serialized by AU wrapper.
class SessionMailbox {
public:
    static_assert(std::atomic<uint32_t>::is_always_lock_free,"session hand-off requires lock-free exchange");
    void publish(const SessionFrame& f) noexcept {
        slots_[back_]=f;
        back_=middle_.exchange(back_|Dirty,std::memory_order_acq_rel)&Index;
    }
    bool hasUpdate()const noexcept{return (middle_.load(std::memory_order_acquire)&Dirty)!=0;}
    bool take(SessionFrame& f) noexcept {
        if(!hasUpdate())return false;
        front_=middle_.exchange(front_,std::memory_order_acq_rel)&Index;
        f=slots_[front_];return true;
    }
private:
    static constexpr uint32_t Dirty=4,Index=3;
    std::array<SessionFrame,3> slots_{};
    std::atomic<uint32_t> middle_{1};
    uint32_t back_{0},front_{2};
};
}
