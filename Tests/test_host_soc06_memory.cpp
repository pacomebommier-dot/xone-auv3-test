// SOC-06 regression: local MEMORY short recalls a filled slot, long overwrites,
// and empty short/long perform first capture. Standalone suite to keep HostEngine
// stack allocations out of the historical MEMORY suite.
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
using namespace xone::host;
static xone::l01::NavImage page(int family) {
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto n=nav.active();const int root=family==4?1:0;
    n.at(291)=root;
    n.at(299+2*root)=1;
    n.at(300+2*root)=0;
    n.at(293+2*root)=1;
    assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
    return n;
}
static SessionImage session() {
    SessionImage s{};s.schema=1;s.config=7;s.revision=1;s.desired.fill(-2);
    for(int o=0;o<28;++o)for(int f=0;f<9;++f)
        s.desired[static_cast<unsigned>(xone::l01::FxS6::addressFor(o,f))]=f<6?50:0;
    return s;
}
static int keyFor(const xone::l01::NavImage& n,int slot){
    for(int p=4;p<20;++p){const auto route=n.route(p);
        if(route.address/1024==5 && route.position==slot)return p;
    }
    assert(false);return -1;
}
static void press(HostEngine& e,int physical,int down,int up){
    for(int note=24;note<40;++note)if(xone::l01::SurfaceInput::physicalControl(note)==physical){
        const uint8_t on[]{0x9e,static_cast<uint8_t>(note),127};
        const uint8_t off[]{0x8e,static_cast<uint8_t>(note),0};
        assert(!e.receive(0,on,3,down,down).emit);
        assert(!e.receive(0,off,3,up,up).emit);
        return;
    }
    assert(false);
}
static void individual(int family,bool initialLong){
    auto e=std::make_unique<HostEngine>();
    const auto n=page(family);
    assert(e->setNavForDiagnostic(n)&&e->restoreSessionBeforeRendering(session()));
    const int key=keyFor(n,0),owner=family==3?0:6;
    xone::l01::BankS6::MemoryRecord record{};
    press(*e,key,10,initialLong?210:11);
    assert(e->readMemory(owner,0,record)&&record.valid&&record.revision==1);
    assert(e->pendingMemorySlot(owner)==-1);
    press(*e,key,230,430); // exactly 200ms -> LONG overwrite
    assert(e->readMemory(owner,0,record)&&record.revision==2);
    assert(e->pendingMemorySlot(owner)==-1);
    press(*e,key,450,451); // filled SHORT -> NEXT_BEAT recall
    assert(e->pendingMemorySlot(owner)==0);
    e->stop(452);
    assert(e->pendingMemorySlot(owner)==-1);
    assert(e->readMemory(owner,0,record)&&record.revision==2);
}
static void filledButDisengaged(){
    auto e=std::make_unique<HostEngine>();
    const auto n=page(3);
    assert(e->setNavForDiagnostic(n)&&e->restoreSessionBeforeRendering(session()));
    const int key=keyFor(n,0);
    press(*e,key,10,11);
    xone::l01::BankS6::MemoryRecord record{};
    assert(e->readMemory(0,0,record)&&record.valid&&record.revision==1);
    assert(e->requestSourceRecall(0,3));
    GridBatch bar{};bar.valid=true;bar.count=1;bar.events[0]={GridKind::Bar,0,4};
    e->onMusicalGrid(bar);
    assert(!e->memoryEngaged(0,0)&&e->readMemory(0,0,record)&&record.revision==1);
    press(*e,key,20,21);
    assert(e->pendingMemorySlot(0)==0);
    assert(e->readMemory(0,0,record)&&record.revision==1);
}
int main(){
    for(int family:{3,4})for(bool initialLong:{false,true})individual(family,initialLong);
    filledButDisengaged();
    std::cout<<"PASS SOC-06 MEMORY INST/YELLOW empty short/long, filled short/long, inactive saved and STOP\n";
}
