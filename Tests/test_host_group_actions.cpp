#include "../Shared/XoneTopPulseBus.hpp"
#include "../Shared/XoneGroupActionGate.hpp"
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void expect(TopPulsePacket p,unsigned api,bool on) {
    assert(p.valid&&p.api==api&&p.on==on);
    assert(p.bytes[0]==(on?0x90:0x80)+6+api/128);
    assert(p.bytes[1]==api%128&&p.bytes[2]==(on?127:0));
}
static void take(TopPulseBus& bus,unsigned api) {
    auto p=bus.next();expect(p,api,true);
    assert(bus.acknowledge(p,true));
    p=bus.next();expect(p,api,false);
    assert(bus.acknowledge(p,true));
}
static xone::l01::NavImage yellow(unsigned page) {
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto n=nav.active();n.at(291)=1;
    if(page<4){n.at(295)=static_cast<int>(page+1);n.at(301)=1;}
    else {n.at(296)=static_cast<int>(page+1);n.at(302)=1;}
    assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
    return n;
}
static xone::l01::NavImage fill(unsigned page) {
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto n=nav.active();n.at(291)=1;n.at(298)=static_cast<int>(page+1);n.at(304)=1;
    assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
    return n;
}
static void physicalDown(HostEngine& host) {
    const std::uint8_t midi[]{0x9E,48,127}; // SPOT physical20, no inferred GROUP gesture
    assert(!host.receive(0,midi,3,0,0).emit);
    assert(host.surfaceHeld(20));
}
int main(){
    TopPulseBus bus;
    assert(TopPulseBus::GroupTotal==72&&TopPulseBus::Total==94);
    for(unsigned page=0;page<8;++page)for(unsigned action=0;action<3;++action) {
        assert(bus.queueGroup(false,page,action));
        take(bus,608+3*page+action);
    }
    for(unsigned page=0;page<8;++page)for(unsigned action=0;action<6;++action) {
        assert(bus.queueGroup(true,page,action));
        take(bus,632+6*page+action);
    }
    assert(!bus.queueGroup(false,8,0)&&!bus.queueGroup(false,0,3));
    assert(!bus.queueGroup(true,8,0)&&!bus.queueGroup(true,0,6));
    assert(bus.queueGroup(false,0,0));
    assert(bus.queue(TopPulseFamily::Init,0));
    expect(bus.next(),608,true);take(bus,608);take(bus,680); // unified FIFO
    assert(bus.queueGroup(true,7,5));
    auto pulse=bus.next();expect(pulse,679,true);
    assert(bus.acknowledge(pulse,false)); // failed ON => debt OFF
    assert(bus.queueGroup(false,2,1)); // pending ON cancelled by STOP
    bus.stop();assert(bus.pending()==1&&bus.hasOwedOff());
    pulse=bus.next();expect(pulse,679,false);
    assert(bus.acknowledge(pulse,false)); // failed OFF stays due
    expect(bus.next(),679,false);
    assert(bus.acknowledge(bus.next(),true));
    assert(bus.pending()==0);
    for(unsigned i=0;i<TopPulseBus::Capacity;++i)
        assert(bus.queueGroup(false,0,0));
    assert(!bus.queueGroup(false,0,0));
    bus.stop();assert(bus.pending()==0); // overflow never overwrites a live OFF
    // Root1 YELLOW and FILL page ownership, with actual S3 validation.
    for(unsigned page=0;page<8;++page) {
        xone::l01::NavS3 nav;assert(nav.initializeDefault());
        auto n=yellow(page);assert(nav.stage(n)&&nav.commit());
        QualifiedGroupAction q{false,page,2,20,nav.revision(),1,true};
        assert(GroupActionGate::authorize(nav,q,true,false));
        q.sourceOwned=false;assert(!GroupActionGate::authorize(nav,q,true,false));
        q.sourceOwned=true;q.navRevision--;assert(!GroupActionGate::authorize(nav,q,true,false));
        q.navRevision=nav.revision();q.page=(page+1)%8;
        assert(!GroupActionGate::authorize(nav,q,true,false)); // cannot target hidden page
        n=fill(page);assert(nav.stage(n)&&nav.commit());
        q={true,page,5,20,nav.revision(),1,true};
        assert(GroupActionGate::authorize(nav,q,true,false));
        q.fill=false;assert(!GroupActionGate::authorize(nav,q,true,false));
    }
    // Genuine host seam: held input alone is never an action.
    HostEngine e;assert(e.setNavForDiagnostic(yellow(5)));physicalDown(e);
    QualifiedGroupAction g{false,5,2,20,e.nativeNavRevisionForQualifiedSource(),
        e.physicalDownEpochForQualifiedSource(20),false};
    assert(!e.acceptQualifiedGroupAction(g)&&e.pendingActionPulses()==0);
    g.sourceOwned=true;
    assert(e.acceptQualifiedGroupAction(g));expect(e.nextActionPulse(),625,true);
    pulse=e.nextActionPulse();e.ackTopPulse(pulse,true,100);
    pulse=e.nextActionPulse();expect(pulse,625,false);
    e.ackTopPulse(pulse,true,100);assert(!e.nextActionPulse().valid);
    g.navRevision--;assert(!e.acceptQualifiedGroupAction(g));
    g.navRevision=e.nativeNavRevisionForQualifiedSource();
    g.page=4;assert(!e.acceptQualifiedGroupAction(g));
    g.page=5;g.sourceOwned=false;assert(!e.acceptQualifiedGroupAction(g));
    g.sourceOwned=true;e.stop(200);
    assert(!e.acceptQualifiedGroupAction(g)); // stale DOWN epoch, despite Held
    assert(e.physicalDownEpochForQualifiedSource(20)==0);
    // Group action requires a real source-owned hold; a missing receiver
    // is not a reason to invent an alternate CC/note/plugin routing.
    HostEngine f;assert(f.setNavForDiagnostic(fill(7)));physicalDown(f);
    g={true,7,5,20,f.nativeNavRevisionForQualifiedSource(),
        f.physicalDownEpochForQualifiedSource(20),true};
    assert(f.acceptQualifiedGroupAction(g));
    expect(f.nextActionPulse(),679,true);
    pulse=f.nextActionPulse();f.ackTopPulse(pulse,false,10);f.stop(20);
    expect(f.nextActionPulse(),679,false);
    f.ackTopPulse(f.nextActionPulse(),true,20);
    assert(!f.nextActionPulse().valid);
    std::cout<<"PASS all S7 GROUP/LONG 72 addresses, captured owner, shared FIFO, STOP and callback debt\n";
}
