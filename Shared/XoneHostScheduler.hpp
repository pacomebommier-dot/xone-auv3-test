#pragma once
// L05.3: fixed-capacity render-thread musical command scheduler.
// Does not emit MIDI or infer BAR when the host clock is absent.
#include "XoneHostGrid.hpp"
#include <array>
#include <cstdint>
#include <type_traits>
namespace xone::host {
enum class ScheduleWhen : unsigned char { NextBeat, NextBar };
struct ScheduledAction {
    std::uint16_t command{},destination{};
    std::uint32_t generation{};
    ScheduleWhen when{ScheduleWhen::NextBar};
    std::uint64_t order{};
};
struct DueAction {
    ScheduledAction action{};
    std::uint32_t sampleOffset{};
    std::int64_t musicalOrdinal{};
};
class HostActionScheduler {
public:
    static constexpr unsigned Capacity=35;
    void stop() noexcept { clear(); ++generation_; }
    void clear() noexcept { count_=0; }
    std::uint32_t generation() const noexcept {return generation_;}
    unsigned pending() const noexcept {return count_;}
    std::uint64_t lastOrder()const noexcept {return nextOrder_;}
    std::uint64_t orderOf(std::uint16_t command,std::uint16_t destination)const noexcept {
        for(unsigned i=0;i<count_;++i)if(pending_[i].command==command&&pending_[i].destination==destination)return pending_[i].order;
        return 0;
    }
    bool enqueue(std::uint16_t command,std::uint16_t destination,ScheduleWhen when) noexcept {
        if(count_==Capacity||nextOrder_==UINT64_MAX)return false;
        pending_[count_++]={command,destination,generation_,when,++nextOrder_};
        return true;
    }
    // Replacement obtains the order of the newest accepted gesture.
    bool replace(std::uint16_t command,std::uint16_t destination,ScheduleWhen when) noexcept {
        bool exists=false;
        for(unsigned i=0;i<count_;++i)if(pending_[i].command==command&&pending_[i].destination==destination)exists=true;
        if((!exists&&count_==Capacity)||nextOrder_==UINT64_MAX)return false;
        cancel(command,destination);return enqueue(command,destination,when);
    }
    void cancel(std::uint16_t command,std::uint16_t destination) noexcept {
        unsigned write=0;
        for(unsigned i=0;i<count_;++i)if(pending_[i].command!=command||pending_[i].destination!=destination)pending_[write++]=pending_[i];
        count_=write;
    }
    // The caller increments generation when switching session/destination; stale actions cannot fire.
    void invalidate() noexcept { ++generation_;clear(); }
    // When clock is missing or has reset, never execute or guess timing.
    // Pending items remain queued during temporary clock loss, unless caller explicitly stops.
    template <typename Consumer>
    unsigned consume(const GridBatch& batch,Consumer&& consumer) noexcept {
        if(!batch.valid || batch.overflow || batch.count>batch.events.size())return 0;
        unsigned fired=0;
        for(unsigned i=0;i<batch.count;++i){
            const auto& edge=batch.events[i];
            const auto cutoff=nextOrder_;std::array<std::uint64_t,Capacity> attempted{};unsigned attempts=0,j=0;
            while(j<count_){
                const auto item=pending_[j];
                bool already=false;for(unsigned a=0;a<attempts;++a)if(attempted[a]==item.order)already=true;
                const bool stale=item.generation!=generation_;
                const bool due=!stale&&!already&&item.order<=cutoff&&(item.when==ScheduleWhen::NextBeat||edge.kind==GridKind::Bar);
                if(!stale&&!due){++j;continue;}
                for(unsigned k=j+1;k<count_;++k)pending_[k-1]=pending_[k];
                --count_;
                if(!due)continue;
                attempted[attempts++]=item.order;
                bool applied=true;
                if constexpr(std::is_same_v<decltype(consumer(DueAction{})),bool>)
                    applied=consumer(DueAction{item,edge.offset,edge.ordinal});
                else consumer(DueAction{item,edge.offset,edge.ordinal});
                if(applied)++fired;
                else if(item.generation==generation_&&count_<Capacity){
                    unsigned insert=0;while(insert<count_&&pending_[insert].order<item.order)++insert;
                    for(unsigned k=count_;k>insert;--k)pending_[k]=pending_[k-1];
                    pending_[insert]=item;++count_;
                }
                // The consumer can cancel earlier entries. Restart by identity,
                // without retrying an attempted item or admitting a fresh one
                // retroactively at this same musical edge.
                j=0;
            }
        }
        return fired;
    }
private:
    std::array<ScheduledAction,Capacity> pending_{};
    unsigned count_{};
    std::uint32_t generation_{1};
    std::uint64_t nextOrder_{};
};
} // namespace xone::host
