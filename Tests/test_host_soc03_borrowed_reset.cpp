// SOC-03: real MIDI RESET SPOT gestures, numeric CC, LED hold/group flash.
// Uses the 4 GOLDEN INST/YELLOW/DRUMS/FILL borrowed FX domains, not
// the effect's ON/OFF; STOP and 119/120ms boundary are proved separately.
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
#include <array>
using namespace xone::host;
struct Strip {
    xone::l01::NavImage nav{};
    std::array<int,3> controls{},cells{},apis{};
    MuteLedColor color{};
};
static int noteFor(int physical) {
    for(int n=40;n<52;++n)
        if(xone::l01::SurfaceInput::physicalControl(n)==physical)return n;
    return -1;
}
static Result edge(HostEngine& h,int physical,bool down,int ms) {
    const int n=noteFor(physical);assert(n>=0);
    const uint8_t b[]{uint8_t(down?0x9E:0x8E),uint8_t(n),uint8_t(down?127:0)};
    return h.receive(0,b,3,ms*48,ms);
}
static Strip strip(int family,int page=1) {
    xone::l01::NavS3 root;assert(root.initializeDefault());
    Strip s{};s.nav=root.active();
    const int side=(family==3||family==5)?0:1;
    s.nav.at(291)=side;
    if(family==3+side){
        s.nav.at(293+2*side)=page;s.nav.at(299+2*side)=1;
    }else {
        assert(family==5+side);
        s.nav.at(297+side)=page;s.nav.at(303+side)=1;
    }
    s.nav.at(292)=5; // RESET overlay, source header remains SOCLE
    assert(xone::l01::NavS3::regenerate(s.nav)&&xone::l01::NavS3::validate(s.nav));
    const int column=family==3+side?1:3;
    for(int k=0;k<3;++k){
        const int p=20+column+4*k;
        const auto r=s.nav.route(p);assert(r.address>=0&&r.address/1024==1);
        const auto t=continuousTarget({r.address-1024,r.position});
        assert(t.valid());
        s.controls[unsigned(k)]=p;s.cells[unsigned(k)]=t.cell;s.apis[unsigned(k)]=t.api;
    }
    s.color=family==4?MuteLedColor::Yellow:family==5?MuteLedColor::Green:MuteLedColor::Red;
    return s;
}
static Strip eq(int side) {
    xone::l01::NavS3 root;assert(root.initializeDefault());
    Strip s{};s.nav=root.active();s.nav.at(291)=side;s.nav.at(292)=0;
    // No sound page and no RESET overlay: the bare EQ SPOT already
    // owns its short/long RESET. Do not treat DRUMS/FILL SOLO like this.
    assert(xone::l01::NavS3::regenerate(s.nav)&&xone::l01::NavS3::validate(s.nav));
    for(int k=0;k<3;++k){
        const int p=20+4*k;const auto r=s.nav.route(p);
        assert(r.address/1024==9);const auto t=continuousTarget({r.address-9*1024,r.position});
        assert(t.valid());s.controls[unsigned(k)]=p;s.cells[unsigned(k)]=t.cell;s.apis[unsigned(k)]=t.api;
    }
    s.color=side?MuteLedColor::Yellow:MuteLedColor::Red;return s;
}
static void setup(HostEngine& h,const Strip& s,bool explicitBase) {
    assert(h.setNavForDiagnostic(s.nav));
    SessionImage live{};live.schema=1;live.config=1;live.revision=1;live.desired.fill(-2);
    for(int c:s.cells)live.desired[unsigned(c)]=64;
    // FX state is independent; a RESET must never toggle this ON switch.
    live.desired[1768]=1;
    assert(h.restoreSessionBeforeRendering(live));
    if(explicitBase){
        ResetDefaults base;base.config=1;
        for(unsigned k=0;k<3;++k)assert(base.set(unsigned(s.apis[k]),17+int(k)*7));
        assert(h.requestResetDefaults(base));h.beginRenderBuffer();
    }
}
static void drain(HostEngine& h,const Strip& s,unsigned from,unsigned howMany,bool explicitBase){
    unsigned n=0;while(true){
        auto p=h.nextSourcePacket();if(!p.valid)break;
        assert(n<howMany);
        const unsigned i=from+n;const int api=s.apis[i];
        const int value=explicitBase?17+int(i)*7:((s.cells[i]>=1744)?63:0);
        assert(p.type==10&&p.api==api&&p.value==value);
        assert(p.bytes[0]==0xB6+api/128&&p.bytes[1]==api%128&&p.bytes[2]==value);
        h.ackSourcePacket(p,true,0);++n;
    }
    assert(n==howMany);
}
static void flashState(HostEngine& h,const Strip& s,int ms,bool litAll){
    const auto v=h.ledImage(ms);
    for(unsigned k=0;k<3;++k){
        const auto led=v[unsigned(s.controls[k]-4)];
        if(litAll){assert(led.known&&led.color==s.color&&!led.blinking);}
        else assert(!led.known||led.color==MuteLedColor::Off);
    }
}
static void test(const Strip& s,bool explicitBase){
    {
        HostEngine h;setup(h,s,explicitBase);
        auto r=edge(h,s.controls[0],true,100);assert(!r.emit);
        const auto led=h.ledImage(101)[unsigned(s.controls[0]-4)];
        assert(led.known&&led.color==s.color&&!led.blinking);
        drain(h,s,0,1,explicitBase);
        r=edge(h,s.controls[0],false,219);assert(!r.emit); // 119ms
        assert(h.desiredLive(s.cells[1]).value==64&&h.desiredLive(s.cells[2]).value==64);
        flashState(h,s,220,false);
        assert(!h.nextSourcePacket().valid);
        assert(h.desiredLive(1768).value==1);
    }
    {
        HostEngine h;setup(h,s,explicitBase);
        auto r=edge(h,s.controls[0],true,100);assert(!r.emit);
        drain(h,s,0,1,explicitBase);
        h.advance(219,219*48);assert(!h.nextSourcePacket().valid);
        h.advance(220,220*48);
        // The touched parameter is not redundantly reset twice.
        drain(h,s,1,2,explicitBase);
        for(unsigned i=0;i<3;++i)
            assert(h.desiredLive(s.cells[i]).value==
                   (explicitBase?17+int(i)*7:s.cells[i]>=1744?63:0));
        flashState(h,s,220,true); // simultaneous group feedback
        r=edge(h,s.controls[0],false,230);assert(!r.emit);
        flashState(h,s,319,true); // retained for exactly 100ms
        flashState(h,s,320,false);
        assert(!h.nextSourcePacket().valid&&h.desiredLive(1768).value==1);
    }
    {
        HostEngine h;setup(h,s,explicitBase);
        edge(h,s.controls[0],true,100);drain(h,s,0,1,explicitBase);
        edge(h,s.controls[0],false,220); // exact 120ms, no timer callback
        drain(h,s,1,2,explicitBase);flashState(h,s,220,true);
        h.advance(400,400*48);assert(!h.nextSourcePacket().valid);
    }
    {
        HostEngine h;setup(h,s,explicitBase);
        edge(h,s.controls[0],true,100);drain(h,s,0,1,explicitBase);
        h.stop(101*48);h.advance(500,500*48);
        edge(h,s.controls[0],false,510);
        assert(!h.nextSourcePacket().valid);flashState(h,s,510,false);
        assert(h.desiredLive(s.cells[1]).value==64);
    }
}
// Verify actual Xone K2 LED MIDI packets, not only semantic ledImage().
static unsigned ledPackets(HostEngine& h,const Strip& s,int ms,bool on){
    h.beginRenderBuffer();h.prepareMuteLeds(ms);
    unsigned cursor=0,count=0;
    for(unsigned n=0;n<136;++n){
        auto packet=h.nextMuteLedPacket(cursor);if(!packet.valid)break;
        for(int physical:s.controls)
            if(packet.target==unsigned(physical-4) &&
               packet.color==unsigned(s.color)-1 && packet.on==on){
                assert(packet.bytes[2]==(on?127:0));++count;
            }
        h.ackMuteLedPacket(packet,true);
    }
    return count;
}
static void verifyLedPackets(const Strip& s){
    HostEngine h;setup(h,s,true);
    // Individual signal ON while the key remains down.
    edge(h,s.controls[0],true,100);drain(h,s,0,1,true);
    assert(ledPackets(h,s,101,true)==1);
    h.advance(220,220*48);drain(h,s,1,2,true);
    // The two peers light simultaneously; touched key was already ON.
    assert(ledPackets(h,s,220,true)==2);
    edge(h,s.controls[0],false,230);
    assert(ledPackets(h,s,319,false)==0);
    assert(ledPackets(h,s,320,false)==3); // after exactly 100ms

    // A new NAV epoch must suppress an old LED pulse, even mid-flash.
    HostEngine other;setup(other,s,true);
    edge(other,s.controls[0],true,100);drain(other,s,0,1,true);
    other.advance(220,220*48);drain(other,s,1,2,true);
    edge(other,s.controls[0],false,230);
    xone::l01::NavS3 root;assert(root.initializeDefault());
    assert(other.setNavForDiagnostic(root.active()));
    assert(ledPackets(other,s,231,true)==0);
}
int main(){
 for(int family:{3,4,5,6})verifyLedPackets(strip(family));
 for(int family:{3,4,5,6})test(strip(family),true);
 for(int family:{3,4,5,6})test(strip(family),false);
 for(int side:{0,1})test(eq(side),false);
 std::cout<<"PASS SOC-03 borrowed FX/EQ RESET individual/group at 119/120ms, BASE overrides, K2 MIDI LED hold/100ms, STOP\n";
}
