#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;

int main() {
    HostEngine engine;
    const int drum=1648, eq=1467;
    assert(engine.desiredLive(drum).value==0 && engine.openingValue(drum)==0);
    assert(engine.desiredLive(eq).value==63 && engine.openingValue(eq)==63);
    assert(engine.desiredLive(1744).value==63 && engine.desiredLive(1756).value==63);
    assert(!engine.desiredLive(1684).known && !engine.nextSourcePacket().valid);
    assert(engine.captureMemory(12,0)); // unedited, real DRUM sound

    auto saved=engine.documentForHost();
    saved.live.desired[drum]=42;
    saved.current.live.desired[drum]=42;
    saved.live.desired[eq]=-2;
    saved.current.live.desired[eq]=-2;
    assert(engine.requestDocumentRestore(saved));
    ResetDefaults defaults;
    defaults.config=1;
    assert(defaults.set(699,51));
    assert(engine.requestResetDefaults(defaults,true));
    engine.beginRenderBuffer();
    assert(engine.desiredLive(drum).value==42 && engine.openingValue(drum)==42);
    assert(engine.desiredLive(eq).value==51 && engine.openingValue(eq)==51);

    bool projectedDrum=false;
    for(;;) {
        const auto packet=engine.nextSourcePacket();
        if(!packet.valid)break;
        assert(packet.api!=699); // Missing legacy field was seeded locally only.
        if(packet.api==811)projectedDrum=packet.value==42;
        engine.ackSourcePacket(packet,true,0);
    }
    assert(projectedDrum);
    std::array<ContinuousTarget,97> targets{};
    targets[0]={drum,811};
    targets[1]={eq,699};
    assert(engine.resetContinuousTargets(targets,2));
    assert(engine.desiredLive(drum).value==0 && engine.openingValue(drum)==42);
    assert(engine.desiredLive(eq).value==51 && engine.openingValue(eq)==51);
    engine.endRenderBuffer();
    engine.beginRenderBuffer();
    assert(engine.openingValue(drum)==42);
    std::cout<<"PASS opening BASE defaults, saved LIVE, explicit RESET and silent legacy completion\n";
}
