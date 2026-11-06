#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;

static void setup(HostEngine& engine,int family,int page) {
    xone::l01::NavS3 nav;
    assert(nav.initializeDefault());
    auto image=nav.active();
    image.at(291)=family;image.at(303+family)=1;image.at(297+family)=page;
    assert(xone::l01::NavS3::regenerate(image)&&engine.setNavForDiagnostic(image));
}
static unsigned flush(HostEngine& engine) {
    unsigned count=0;
    for(;;){
        const auto packet=engine.nextSourcePacket();
        if(!packet.valid)return count;
        ++count;engine.ackSourcePacket(packet,true,0);
    }
}
static void cc(HostEngine& engine,int number,int value,int ms) {
    const uint8_t bytes[]{0xbe,static_cast<uint8_t>(number),static_cast<uint8_t>(value)};
    const auto result=engine.receive(0,bytes,3,ms,ms);
    if(result.emit)engine.sent(result,0);
}
static void edge(HostEngine& engine,int physical,bool down,int ms) {
    int note=-1;
    for(int candidate=24;candidate<52;++candidate)
        if(xone::l01::SurfaceInput::physicalControl(candidate)==physical)note=candidate;
    assert(note>=0);
    const uint8_t bytes[]{static_cast<uint8_t>(down?0x9e:0x8e),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};
    assert(!engine.receive(0,bytes,3,ms,ms).emit);
}
static GridBatch grid(GridKind kind) {
    GridBatch batch{};batch.valid=true;batch.count=1;batch.events[0]={kind,0,4};return batch;
}

int main() {
    for(int family=0;family<2;++family){
        HostEngine group;
        setup(group,family,7);
        const int start=1648+48*family;
        auto live=group.sessionForHost();
        for(int sound=0;sound<6;++sound)live.desired[start+6*sound]=20+10*sound;
        assert(group.restoreSessionBeforeRendering(live));
        cc(group,6,64,1); // col3 row1 is sound FX parameter 1
        assert(flush(group)==0);
        for(int sound=0;sound<6;++sound)assert(group.desiredLive(start+6*sound).value==20+10*sound);
        cc(group,6,96,2);
        assert(flush(group)==6);
        assert(group.desiredLive(start).value<group.desiredLive(start+30).value);
        cc(group,6,127,3);assert(flush(group)==6);
        for(int sound=0;sound<6;++sound)assert(group.desiredLive(start+6*sound).value==127);
        cc(group,6,90,4);assert(flush(group)==6);
        for(int sound=0;sound<6;++sound)assert(group.desiredLive(start+6*sound).value==90);
        edge(group,22,true,5);edge(group,22,false,6);assert(flush(group)==6);
        for(int sound=0;sound<6;++sound)assert(group.desiredLive(start+6*sound).value==0);

        for(int page:{1,7,8}){
            HostEngine engine;setup(engine,family,page);
            const auto before=engine.sessionForHost().desired;
            edge(engine,4,true,1);edge(engine,4,false,2);
            unsigned cursor=0,count=0;
            assert(!engine.nextSoftPacket(cursor).valid);
            engine.onMusicalGrid(grid(GridKind::Bar));
            for(;;){
                const auto packet=engine.nextSoftPacket(cursor);
                if(!packet.valid)break;
                assert(packet.on&&packet.index%4==0&&packet.index/4%8<6);
                if(page!=8)assert(packet.index/32==static_cast<unsigned>(family));
                engine.ackSoftPacket(packet,true);
                ++count;
            }
            assert(count==(page==8?12u:page==7?6u:1u));
            engine.endRenderBuffer();
            assert(engine.sessionForHost().desired==before);
        }

        HostEngine aliases;setup(aliases,family,8);
        unsigned mirrored=0;
        for(int dial=4;dial<16;++dial){
            const auto target=continuousTarget(aliases.routeForControl(36+dial));
            if(target.cell<1467||target.cell>1472)continue;
            ++mirrored;
            assert(target.api==target.cell-768);
            cc(aliases,dial,63,100*dial);cc(aliases,dial,90,100*dial+1);
            assert(aliases.desiredLive(target.cell).value==90);
            edge(aliases,16+dial,true,100*dial+2);edge(aliases,16+dial,false,100*dial+3);
            assert(aliases.desiredLive(target.cell).value==63&&flush(aliases)==1);
        }
        assert(mirrored==6);
    }
    HostEngine memory;
    setup(memory,1,8);
    auto live=memory.sessionForHost();
    live.desired[1648]=42;
    assert(memory.restoreSessionBeforeRendering(live)&&memory.captureMemory(12,0));
    live.desired[1648]=37;live.desired[1696]=61;
    assert(memory.restoreSessionBeforeRendering(live));
    std::array<ContinuousTarget,97> resets{};
    resets[0]={1648,811};resets[1]={1696,859};
    assert(memory.resetContinuousTargets(resets,2)&&flush(memory)==2);
    assert(memory.desiredLive(1648).value==0&&memory.desiredLive(1696).value==0);
    int button=-1, emptyButton=-1;
    for(int p=4;p<20;++p){
        const auto route=memory.routeForControl(p);
        if(route.address/1024!=5)continue;
        if(route.position==0)button=p;
        if(route.position==1)emptyButton=p;
    }
    assert(button>=0&&emptyButton>=0);
    assert(memory.ledImage()[static_cast<unsigned>(button-4)].color==MuteLedColor::Yellow);
    edge(memory,button,true,10);
    assert(flush(memory)==0&&!memory.ledImage()[static_cast<unsigned>(button-4)].blinking);
    edge(memory,button,false,11);
    assert(memory.ledImage()[static_cast<unsigned>(button-4)].blinking);
    memory.onMusicalGrid(grid(GridKind::Beat));
    assert(flush(memory)==108);
    assert(memory.desiredLive(1648).value==42&&memory.desiredLive(1696).value==61);
    xone::l01::BankS6::MemoryRecord record{};
    assert(memory.readMemory(20,0,record)&&!record.valid);
    edge(memory,emptyButton,true,20);edge(memory,emptyButton,false,21);
    for(int family=0;family<2;++family)for(int sound=0;sound<6;++sound)
        assert(memory.readMemory(12+8*family+sound,1,record)&&record.valid);
    assert(memory.readMemory(27,1,record)&&!record.valid);
    assert(memory.ledImage()[static_cast<unsigned>(emptyButton-4)].color==MuteLedColor::Red);
    assert(memory.readMemory(12,1,record)&&record.revision==1&&record.values[0]==42);
    assert(memory.resetContinuousTargets(resets,2)&&flush(memory)==2);
    edge(memory,emptyButton,true,30);
    memory.onMusicalGrid(grid(GridKind::Beat));
    assert(memory.desiredLive(1648).value==0&&flush(memory)==0);
    edge(memory,emptyButton,false,230);
    assert(memory.readMemory(12,1,record)&&record.revision==2&&record.values[0]==0);
    assert(memory.readMemory(20,1,record)&&record.revision==2&&record.values[0]==0);
    assert(flush(memory)==0); // replacement does not recall
    HostEngine individual;setup(individual,0,1);
    int single=-1, singleEmpty=-1;
    for(int p=4;p<20;++p){
        const auto route=individual.routeForControl(p);
        if(route.address/1024!=5)continue;
        if(route.position==0)single=p;
        if(route.position==1)singleEmpty=p;
    }
    assert(single>=0&&singleEmpty>=0);
    edge(individual,single,true,1);edge(individual,single,false,2);
    assert(individual.readMemory(12,0,record)&&record.revision==1);
    edge(individual,single,true,3);edge(individual,single,false,203);
    assert(individual.readMemory(12,0,record)&&record.revision==2);
    edge(individual,single,true,204);individual.stop(205);edge(individual,single,false,450);
    assert(individual.readMemory(12,0,record)&&record.revision==2);
    edge(individual,singleEmpty,true,451);edge(individual,singleEmpty,false,651);
    assert(individual.readMemory(12,1,record)&&record.valid&&record.revision==1);
    assert(individual.memoryEngaged(12,1)&&individual.pendingMemorySlot(12)==-1);
    std::cout<<"PASS DRUM/FILL page 7 fan-out, RESET, shared page 8 aliases and SOFT groups\n";
}
