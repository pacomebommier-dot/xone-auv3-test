#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace xone::host;
static xone::l01::NavImage page(bool fill,int choice) {
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto n=nav.active();n.at(291)=1;
    if(fill){n.at(304)=1;n.at(298)=choice;}
    else if(choice<=4){n.at(301)=1;n.at(295)=choice;}
    else{n.at(302)=1;n.at(296)=choice;}
    assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
    return n;
}
static int note(unsigned physical) {
    for(int i=40;i<52;++i)
        if(xone::l01::SurfaceInput::physicalControl(i)==static_cast<int>(physical))return i;
    return -1;
}
static void edge(HostEngine& h,unsigned physical,bool down,int ms) {
    const int n=note(physical);assert(n>=0);
    const std::uint8_t bytes[]{static_cast<std::uint8_t>(down?0x9E:0x8E),
        static_cast<std::uint8_t>(n),static_cast<std::uint8_t>(down?127:0)};
    assert(!h.receive(0,bytes,3,48*ms,ms).emit);
}
static void expect(HostEngine& h,unsigned api) {
    const auto on=h.nextActionPulse();
    assert(on.valid&&on.on&&on.api==api&&
        on.bytes[0]==0x90+6+api/128&&on.bytes[1]==api%128&&on.bytes[2]==127);
    h.ackTopPulse(on,true,0);
    const auto off=h.nextActionPulse();
    assert(off.valid&&!off.on&&off.api==api&&off.bytes[0]==0x80+6+api/128);
    h.ackTopPulse(off,true,0);
    assert(!h.nextActionPulse().valid);
}
int main(){
    // YELLOW6 SPOT1 third button: 608+5*3+2=625.
    HostEngine y;assert(y.setNavForDiagnostic(page(false,6)));
    edge(y,28,true,0);
    assert(y.pendingActionPulses()==0); // no early note
    y.advance(119,119*48);
    assert(y.pendingActionPulses()==0);
    y.advance(120,120*48);
    assert(y.pendingActionPulses()==1);
    y.advance(160,160*48);
    assert(y.pendingActionPulses()==1); // not replayed by timer
    edge(y,28,false,161);
    assert(y.pendingActionPulses()==1); // no duplicate release
    expect(y,625);
    // FILL8 SPOT4 sixth action on physical 31: 632+7*6+5=679.
    // The release itself crosses 120 ms; no fabricated timer is needed.
    HostEngine f;assert(f.setNavForDiagnostic(page(true,8)));
    edge(f,31,true,200);
    edge(f,31,false,320);
    assert(f.pendingActionPulses()==1);
    expect(f,679);
    // YELLOW1: release below 120ms must not make a GROUP action.
    HostEngine shortPress;assert(shortPress.setNavForDiagnostic(page(false,1)));
    edge(shortPress,20,true,500);
    edge(shortPress,20,false,619);
    assert(shortPress.pendingActionPulses()==0);
    // Root1 FILL mode cannot use hidden SPOT1/2 as a GROUP source.
    HostEngine hidden;assert(hidden.setNavForDiagnostic(page(true,1)));
    edge(hidden,20,true,0);hidden.advance(200,9600);edge(hidden,20,false,201);
    assert(hidden.pendingActionPulses()==0);
    // Physical STOP invalidates held epoch, even if real UP follows later.
    HostEngine stopped;assert(stopped.setNavForDiagnostic(page(false,1)));
    edge(stopped,20,true,10);stopped.stop(11*48);
    stopped.advance(210,210*48);edge(stopped,20,false,211);
    assert(stopped.pendingActionPulses()==0);
    // A new BOT navigation while a SPOT is held revokes the captured route.
    HostEngine nav;assert(nav.setNavForDiagnostic(page(false,1)));
    edge(nav,20,true,100);
    const std::uint8_t bd[]{0x9E,15,127},bu[]{0x8E,15,0};
    assert(!nav.receive(0,bd,3,105*48,105).emit);
    assert(!nav.receive(0,bu,3,106*48,106).emit);
    nav.advance(240,240*48);
    edge(nav,20,false,241);
    assert(nav.pendingActionPulses()==0);
    // All GROUP addresses can be triggered with the 3/6 physical positions.
    for(unsigned pageIndex=0;pageIndex<8;++pageIndex){
        for(unsigned action=0;action<3;++action){
            HostEngine h;assert(h.setNavForDiagnostic(page(false,static_cast<int>(pageIndex+1))));
            const unsigned physical=20+4*action;
            edge(h,physical,true,0);h.advance(120,120*48);
            assert(h.pendingActionPulses()==1);
            expect(h,608+3*pageIndex+action);
        }
        for(unsigned action=0;action<6;++action){
            HostEngine h;assert(h.setNavForDiagnostic(page(true,static_cast<int>(pageIndex+1))));
            const unsigned physical=22+4*(action%3)+(action>=3?1:0);
            edge(h,physical,true,0);h.advance(120,120*48);
            assert(h.pendingActionPulses()==1);
            expect(h,632+6*pageIndex+action);
        }
    }
    std::cout<<"PASS GROUP physical 72/72 S3 routes, 120ms edge/timer, STOP, no hidden page, no duplicates\n";
}
