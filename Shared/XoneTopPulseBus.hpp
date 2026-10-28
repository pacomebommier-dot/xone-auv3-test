#pragma once
// S7 V1.2 nonpersistent ACTION bus, MIDI7..14:
// GROUP/LONG NOTE608..679 and TOP NOTE680..701 share ONE ordered FIFO.
// ACTION impulses are not persistent nominal NOTE states; never deduplicate.
// A failed ON callback may have reached the receiver: always close with OFF.
// An OFF failure is owed at the same captured API address until acknowledged.
#include <array>
#include <cstdint>
namespace xone::host {
enum class TopPulseFamily : std::uint8_t { Init, Drums, Yellow, Fill };
struct TopPulsePacket {
    bool valid{},on{};
    unsigned serial{},api{};
    std::uint8_t bytes[3]{};
};
class TopPulseBus {
public:
    static constexpr unsigned Capacity=32;
    static constexpr unsigned TopTotal=22;
    static constexpr unsigned GroupTotal=72;
    static constexpr unsigned Total=TopTotal+GroupTotal;
    // The true S3 group/long gesture owner must be qualified by the caller;
    // this function only translates the stable S7 identity and queues it.
    // page is zero-based 0..7; action is zero-based 0..2 or 0..5.
    bool queueGroup(bool fill,unsigned page,unsigned action) noexcept {
        if(page>=8||action>=(fill?6u:3u))return false;
        return append((fill?632u:608u)+(fill?6u:3u)*page+action);
    }
    bool queue(TopPulseFamily family,unsigned member) noexcept {
        const unsigned f=static_cast<unsigned>(family);
        const unsigned limits[4]{4,8,2,8};
        const unsigned offsets[4]{0,4,12,14};
        if(f>=4 || member>=limits[f])return false;
        return append(680+offsets[f]+member);
    }
    unsigned pending() const noexcept {return count_;}
    bool hasOwedOff() const noexcept {
        for(unsigned n=0;n<count_;++n)if(entries_[(read_+n)%Capacity].onAttempted)return true;
        return false;
    }
    TopPulsePacket next() const noexcept {
        if(!count_)return {};
        const auto& front=entries_[read_];
        const bool on=!front.onAttempted;
        const unsigned api=front.api;
        return {true,on,front.serial,api,{
            static_cast<std::uint8_t>((on?0x90:0x80)+6+api/128),
            static_cast<std::uint8_t>(api%128),
            static_cast<std::uint8_t>(on?127:0)}};
    }
    // Called only after invoking the MIDI output callback. For a failed ON,
    // the receiver might still have heard the note, so OFF must be emitted.
    // A failed OFF never consumes this head or unlocks later pulses.
    bool acknowledge(TopPulsePacket packet,bool success) noexcept {
        if(!packet.valid || !count_)return false;
        auto& front=entries_[read_];
        if(packet.serial!=front.serial || packet.api!=front.api ||
           packet.on==front.onAttempted)return false;
        if(packet.on){front.onAttempted=true;return true;}
        if(!success)return true;
        read_=(read_+1)%Capacity;
        --count_;return true;
    }
    // STOP drops unattempted ON pulses and retains *all* owed OFFs in order.
    // It never fabricates an UP or replays old pulses at the next start.
    void stop() noexcept {
        std::array<Entry,Capacity> kept{};
        unsigned n=0;
        for(unsigned i=0;i<count_;++i){
            const auto& entry=entries_[(read_+i)%Capacity];
            if(entry.onAttempted)kept[n++]=entry;
        }
        entries_=kept;count_=n;read_=0;
    }
private:
    bool append(unsigned api) noexcept {
        if(count_>=Capacity)return false;
        entries_[(read_+count_)%Capacity]={api,++serial_,false};
        ++count_;return true;
    }
    struct Entry {unsigned api{},serial{};bool onAttempted{};};
    std::array<Entry,Capacity> entries_{};
    unsigned read_{},count_{},serial_{};
};
} // namespace xone::host
