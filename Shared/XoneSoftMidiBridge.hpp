#pragma once
// S7 V1.2 SOFT: 8 DRUMS pages + 8 FILL pages, four choices per page.
// Ownership is decoded from the captured S3 NAV route. Applied changes occur
// only on an authenticated BAR. This is domain state, NOT another clock/scheduler.
// MIDI callback success is not an acknowledgement by an external instrument.
#include "XoneHostGrid.hpp"
#include "../Core/NavS3.hpp"
#include <array>
#include <cstdint>
namespace xone::host {
struct SoftMidiPacket {
    bool valid{},on{};unsigned index{};std::uint8_t bytes[3]{};
};
class SoftMidiBridge {
public:
    SoftMidiBridge() noexcept {active_.fill(-1);pending_.fill(-1);}
    // Physical K2 INST1 column: controls 4,8,12,16. No inferred page
    // from an UNKNOWN nav. S3 @zm maps DRUMS to kind5 (kind6/7 for page8)
    // and FILL to kind8; the route is captured at DOWN.
    bool requestFromRoute(int physical,const l01::NavRoute& route,int root) noexcept {
        if(physical<4||physical>16||(physical-4)%4!=0||
           root<0||root>1||route.address<0||route.position<0||route.position>3)return false;
        const int kind=route.address/1024;
        const int family=(route.address%1024)/32;
        const int page=(route.address%1024)%32;
        if(route.position!=(physical-4)/4)return false;
        int owner=-1;
        if(family==3+root && page>=5&&page<=8 &&
           kind==(page==8?6+root:5))
            owner=4*root+page-5;
        else if(kind==8&&family==5+root&&page>=1&&page<=8)
            owner=8+page-1;
        if(owner<0)return false;
        const int choice=route.position;
        pending_[static_cast<unsigned>(owner)]=choice;return true;
    }
    bool requestVerified(unsigned family,unsigned page,unsigned choice) noexcept {
        if(family>1||page>=8||choice>=4)return false;
        pending_[family*8+page]=static_cast<int>(choice);return true;
    }
    // STOP cancels uncommitted choices; it never synthesizes a physical UP
    // or invents a transport BAR. Applied selections remain logical state.
    void stop() noexcept {pending_.fill(-1);sounding_.fill(false);lastBar_=-1;}
    // STOP retains the logical choice but disarms its output; all possibly
    // delivered ONs receive an owed OFF, without replaying ON at reopen.
    bool hasOutstandingNotes()const noexcept {
        for(unsigned i=0;i<64;++i)if(possiblyOn_[i]||needsOff_[i])return true;
        return false;
    }
    unsigned pendingCount()const noexcept {
        unsigned n=0;for(int x:pending_)if(x>=0)++n;return n;
    }
    int active(unsigned family,unsigned page)const noexcept {
        return family<2&&page<8?active_[family*8+page]:-1;
    }
    unsigned observe(const GridBatch& grid) noexcept {
        if(!grid.valid||grid.overflow||grid.count>grid.events.size())return 0;
        unsigned changes=0;
        for(unsigned e=0;e<grid.count;++e){
            const auto& edge=grid.events[e];
            if(edge.kind!=GridKind::Bar||edge.ordinal<0||edge.ordinal==lastBar_)continue;
            lastBar_=edge.ordinal;
            for(unsigned page=0;page<16;++page){
                if(pending_[page]<0)continue;
                const auto choice=static_cast<unsigned>(pending_[page]);
                pending_[page]=-1;
                if(active_[page]>=0) {
                    const unsigned previous=4*page+static_cast<unsigned>(active_[page]);
                    // Even a repeated active/valid choice is a BAR rearm:
                    // release its old selection before a new ON.
                    if(possiblyOn_[previous])needsOff_[previous]=true;
                }
                active_[page]=static_cast<int>(choice);
                sounding_[page]=true;
                ++changes;
            }
        }
        return changes;
    }
    // Call first for failed old OFFs, then desired ONs. The per-page order
    // never permits a new ON to precede an unacknowledged old OFF.
    SoftMidiPacket next(unsigned& cursor) const noexcept {
        for(unsigned pass=0;pass<2;++pass){
            for(unsigned i=cursor;i<64;++i){
                const bool desired=sounding_[i/4]&&active_[i/4]==static_cast<int>(i%4);
                const bool off=needsOff_[i] || (possiblyOn_[i]&&!desired);
                if(pass==0 && off)return make(i,false);
                if(pass==1 && desired&&!confirmedOn_[i]&&!hasOwedOff(i/4))return make(i,true);
            }
        }
        return {};
    }
    void confirm(const SoftMidiPacket& packet,bool success) noexcept {
        if(!packet.valid||packet.index>=64)return;
        const unsigned i=packet.index;
        if(packet.on){
            // Failed ON may have reached the receiver: its later OFF is owed.
            possiblyOn_[i]=true;
            if(success)confirmedOn_[i]=true;
        }else if(success){
            possiblyOn_[i]=false;confirmedOn_[i]=false;needsOff_[i]=false;
        }
    }
private:
    bool hasOwedOff(unsigned page)const noexcept {
        for(unsigned j=4*page;j<4*page+4;++j)
            if(needsOff_[j]||(possiblyOn_[j]&&(!sounding_[page]||active_[page]!=static_cast<int>(j%4))))return true;
        return false;
    }
    static SoftMidiPacket make(unsigned index,bool on)noexcept {
        const unsigned api=544+index;
        return {true,on,index,{static_cast<std::uint8_t>((on?0x90:0x80)+6+api/128),
                   static_cast<std::uint8_t>(api%128),
                   static_cast<std::uint8_t>(on?127:0)}};
    }
    std::array<int,16> active_{},pending_{};
    std::array<bool,16> sounding_{};
    std::array<bool,64> possiblyOn_{},confirmedOn_{},needsOff_{};
    std::int64_t lastBar_{-1};
};
} // namespace xone::host
