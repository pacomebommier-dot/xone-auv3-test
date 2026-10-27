#include "../Shared/XoneHostScheduler.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace xone::host;
static GridBatch batch(bool bar,unsigned at=100) {
    GridBatch b{};b.valid=true;b.count=1;
    b.events[0]={bar?GridKind::Bar:GridKind::Beat,at,bar?4:3};
    return b;
}
int main(){
    HostActionScheduler s;
    std::vector<DueAction> out;
    auto sink=[&](DueAction d){out.push_back(d);};
    assert(s.enqueue(11,2,ScheduleWhen::NextBar));
    assert(s.enqueue(12,3,ScheduleWhen::NextBeat));
    assert(s.consume(batch(false),sink)==1 && out[0].action.command==12);
    assert(out[0].sampleOffset==100 && s.pending()==1);
    assert(s.consume(batch(true,220),sink)==1 && out[1].action.command==11);
    assert(out[1].sampleOffset==220 && out[1].musicalOrdinal==4);
    assert(s.pending()==0);
    assert(s.enqueue(13,4,ScheduleWhen::NextBar));
    GridBatch absent{};assert(s.consume(absent,sink)==0&&s.pending()==1);
    GridBatch overflow=batch(true);overflow.overflow=true;
    assert(s.consume(overflow,sink)==0&&s.pending()==1);
    s.invalidate();assert(s.pending()==0);
    assert(s.consume(batch(true),sink)==0);
    assert(s.enqueue(14,2,ScheduleWhen::NextBar));
    s.stop();assert(s.pending()==0);
    assert(s.consume(batch(true),sink)==0);
    for(unsigned i=0;i<HostActionScheduler::Capacity;++i)
        assert(s.enqueue(static_cast<std::uint16_t>(i),0,ScheduleWhen::NextBar));
    assert(!s.enqueue(900,0,ScheduleWhen::NextBar));
    assert(s.consume(batch(false),sink)==0);
    assert(s.consume(batch(true),sink)==HostActionScheduler::Capacity);
    assert(s.pending()==0);
    // Re-entrancy: a consumer can safely queue a different action for a future boundary?
    // Not guaranteed; render-thread callers must enqueue outside consume.
    std::cout<<"PASS fixed scheduler BAR/BEAT, stop, cancel, missing clock, overflow, capacity\n";
}
