#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
int main() {
    const int cells[]{1546,1547,1550,1476,1744,1747,1750,1753,1745,1748,1751,1754,1746,1749,1752,1755,1328,1329,1332,1336};
    const int api[]{709,710,713,708,907,910,913,916,908,911,914,917,909,912,915,918,560,561,564,568};
    // No fixture: each physical absolute CC supplies its own initial LIVE
    // value and exact normative S7 virtual MIDI output (not a RESET zero).
    HostEngine e;
    for(int k=0;k<20;++k){
        const int raw=20+k;
        uint8_t input[]{0xBE,static_cast<uint8_t>(k),static_cast<uint8_t>(raw)};
        const auto r=e.receive(0,input,3,k,k);
        assert(r.emit && r.musical && r.api==api[k]);
        assert(r.bytes[0]==0xB0+6+api[k]/128);
        assert(r.bytes[1]==api[k]%128 && r.bytes[2]==raw);
        const auto desired=e.desiredLive(cells[k]);
        assert(desired.known && desired.value==raw && e.observed()==-2);
        e.sent(r,0);
    }
    // Without a session or instrument, a failed/no-output TX remains retryable.
    HostEngine unconfiguredRetry;
    uint8_t cc[]{0xBE,16,64};
    auto fresh=unconfiguredRetry.receive(0,cc,3,0,0);
    assert(fresh.emit&&fresh.bytes[0]==0xBA&&fresh.bytes[1]==48&&fresh.bytes[2]==64);
    unconfiguredRetry.sent(fresh,-1);
    fresh=unconfiguredRetry.receive(0,cc,3,1,1);assert(fresh.emit);
    unconfiguredRetry.noOutput(fresh);
    fresh=unconfiguredRetry.receive(0,cc,3,2,2);assert(fresh.emit);
    unconfiguredRetry.sent(fresh,0);
    assert(!unconfiguredRetry.receive(0,cc,3,3,3).emit);
    HostEngine ready;
    xone::l01::LiveState::SessionImage image{};
    image.schema=1;image.config=1;image.revision=1;image.desired.fill(-2);
    for(int cell:cells)image.desired[cell]=64;
    assert(ready.restoreSessionBeforeRendering(image));
    for(int k=0;k<20;++k) {
        uint8_t input[]{0xBE,static_cast<uint8_t>(k),64};
        auto r=ready.receive(0,input,3,k,k);
        assert(r.emit && r.bytes[0]==0xB0+6+api[k]/128 && r.bytes[1]==api[k]%128 && r.bytes[2]==64);
        ready.sent(r,0);
    }
    cc[2]=100;
    auto r=ready.receive(0,cc,3,20,20);
    assert(r.emit && r.bytes[0]==0xBA && r.bytes[1]==48 && r.bytes[2]==100);
    assert(ready.desiredLive(1328).value==100 && ready.observed()==-2);
    ready.sent(r,0);
    assert(!ready.receive(0,cc,3,21,21).emit); // S4 dedup
    assert(!ready.restoreSessionBeforeRendering(image)); // cannot replace a running core
    HostEngine pickup;
    assert(pickup.restoreSessionBeforeRendering(image));
    cc[2]=0;assert(!pickup.receive(0,cc,3,0,0).emit); // far from remembered64
    cc[2]=64;assert(pickup.receive(0,cc,3,200,200).emit); // slow pickup reaches target
    pickup.stop(201);
    HostEngine failed;
    assert(failed.restoreSessionBeforeRendering(image));
    cc[2]=64;
    auto retry=failed.receive(0,cc,3,0,0);assert(retry.emit);
    failed.sent(retry,-1);
    retry=failed.receive(0,cc,3,1,1);assert(retry.emit);
    failed.noOutput(retry);
    retry=failed.receive(0,cc,3,2,2);assert(retry.emit);
    failed.sent(retry,0);
    assert(!failed.receive(0,cc,3,3,3).emit);
    ContinuousPickup rapid;
    assert(rapid.consider(0,0,64,0,0)==-1);
    assert(rapid.consider(0,0,64,60,10)==-1);
    assert(rapid.consider(0,0,64,61,20)==-1);
    assert(rapid.consider(0,0,64,60,30)==60); // source direction reversal acquisition
    ContinuousPickup catchup;
    for(int i=0;i<99;++i)assert(catchup.consider(0,0,64,0,i*2)==-1);
    assert(catchup.consider(0,0,64,0,198)==58); // source 100th attempt, 1/10 catch-up

    assert(pickup.desiredLive(1328).value==64);
    std::cout<<"PASS host physical CC->source target->pickup->LIVE->S4 MIDI7..14, UNKNOWN, no observed inference\n";
}
