#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;

static void edge(HostEngine& engine,int note,bool down,int ms) {
    const uint8_t bytes[]{uint8_t(down?0x9e:0x8e),uint8_t(note),uint8_t(down?127:0)};
    assert(!engine.receive(0,bytes,3,ms,ms).emit);
}
static void drain(HostEngine& engine) {
    for(;;) {
        const auto packet=engine.nextSourcePacket();
        if(!packet.valid)return;
        engine.ackSourcePacket(packet,true,0);
    }
}
static void bar(HostEngine& engine,int ordinal) {
    GridBatch batch{};
    batch.valid=true;batch.count=1;batch.events[0]={GridKind::Bar,0,ordinal};
    engine.onMusicalGrid(batch);
}

int main() {
    HostEngine engine;
    auto live=engine.sessionForHost();
    for(unsigned f=0;f<60;++f)if(SourcePatchBank::included(f))
        live.desired[SourcePatchBank::cell(0,f)]=f<18||f>=42?17:0;
    for(int cell=1552;cell<1576;++cell)live.desired[cell]=77;
    for(int cell=1768;cell<1780;++cell)live.desired[cell]=1;
    live.desired[1546]=31;
    assert(engine.restoreSessionBeforeRendering(live));
    edge(engine,24,true,1);edge(engine,24,false,2); // INST1 CUE page
    edge(engine,25,true,10);
    assert(engine.pendingCueLoad()==0 && engine.sourceRecallStatus(0)==HostEngine::SourceRecallStatus::None);
    bar(engine,4);
    edge(engine,25,false,210);
    assert(engine.pendingCueLoad()==0 && !engine.nextSourcePacket().valid);
    const unsigned led=static_cast<unsigned>(xone::l01::SurfaceInput::physicalControl(25)-4);
    assert(engine.ledImage(210)[led].blinking);
    assert(!engine.ledImage(810)[led].blinking);
    engine.endRenderBuffer();
    auto saved=engine.documentForHost();
    assert(saved.sourceBases[0].valid && saved.sourceBases[0].values[36]==77);
    assert(saved.sourceBases[0].values[60]==1 && saved.sourceBases[0].values[72]==31);

    saved.live.desired[1552]=9;saved.current.live.desired[1552]=9;
    DocumentBytes bytes;
    assert(encodeDocument(saved,bytes)&&bytes[4]==3);
    DocumentImage reopened;
    assert(decodeDocument(bytes.data(),bytes.size(),reopened));
    assert(reopened.sourceBases[0].values[36]==77 && reopened.live.desired[1552]==9);
    HostEngine restored;
    assert(restored.requestDocumentRestore(reopened));
    restored.beginRenderBuffer();drain(restored);
    assert(restored.desiredLive(1552).value==9);
    assert(restored.ledImage()[led].color==MuteLedColor::Yellow);
    edge(restored,25,true,300);bar(restored,8);
    assert(restored.desiredLive(1552).value==9);
    edge(restored,25,false,301);
    assert(restored.sourceRecallStatus(0)==HostEngine::SourceRecallStatus::Pending);
    bar(restored,12);
    assert(restored.desiredLive(1552).value==77);
    assert(restored.desiredLive(1768).value==1 && restored.desiredLive(1546).value==31);
    drain(restored);
    const auto cue=restored.nextCueLoadPacket();
    assert(cue.valid);
    restored.ackCueLoadPacket(cue,true,302);
    assert(restored.ledImage()[led].color==MuteLedColor::Green);
    std::array<ContinuousTarget,97> reset{};
    reset[0]={1552,715};
    assert(restored.resetContinuousTargets(reset,1));
    assert(restored.nextSourcePacket().value==77);
    drain(restored);
    edge(restored,25,true,400);restored.stop(401);edge(restored,25,false,650);
    assert(restored.pendingCueLoad()==0&&!restored.nextSourcePacket().valid);
    std::cout<<"PASS physical CUE4 short/long, full BASE persistence, FX recall and STOP\n";
}
