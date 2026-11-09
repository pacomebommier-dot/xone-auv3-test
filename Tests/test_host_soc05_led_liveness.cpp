// SOC-05: "LED frozen after CC" — deterministic K2 wire-level liveness test.
// The GOLDEN witness lasts 50ms, locks for 100ms, and never owns its base LED.
// A failure to transmit ON/OFF must recover on a later render with no CC input.
#include "../Shared/XoneHostEngine.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace xone::host;

struct K2 {
    std::array<std::array<bool,3>,34> lit{};
    std::array<unsigned,34> packets{};
    unsigned failures{};
    void accept(const MuteLedOutput::Packet& p) {
        assert(p.valid && p.target<34 && p.color<3);
        const auto binding=xone::l01::ledBinding(static_cast<int>(p.target),
                                                static_cast<int>(p.color+1));
        assert(p.bytes[0]==static_cast<unsigned>(p.on?binding.onStatus:binding.offStatus));
        assert(p.bytes[1]==static_cast<unsigned>(binding.note));
        assert(p.bytes[2]==(p.on?127:0));
        lit[p.target][p.color]=p.on;
        ++packets[p.target];
    }
    bool has(unsigned target,unsigned color)const{return lit[target][color];}
    void only(unsigned target,int color)const{
        for(unsigned c=0;c<3;++c)assert(has(target,c)==(int(c)==color));
    }
};
static void render(MuteLedOutput& out,K2& k2,
                   const MuteLedOutput::Image& image,int64_t ms,
                   int failTarget=-1,int failColor=-1,bool failOn=false) {
    out.beginBuffer();out.observeFull(image,ms);
    unsigned cursor=0,steps=0;
    for(;steps<136;++steps) {
        const auto p=out.next(cursor);if(!p.valid)break;
        const bool fail=static_cast<int>(p.target)==failTarget &&
                        static_cast<int>(p.color)==failColor &&
                        p.on==failOn;
        if(!fail)k2.accept(p);
        else ++k2.failures;
        out.confirm(p,!fail);
        if(fail)cursor=p.target+1; // actual AU callback fail policy
    }
    assert(steps<136 && "LED liveness bounded per audio block");
}
static void hostRender(HostEngine& host,K2& k2,int64_t ms,
                       int failTarget=-1,int failColor=-1,bool failOn=false) {
    host.beginRenderBuffer();
    host.prepareMuteLeds(ms);
    unsigned cursor=0,steps=0;
    for(;steps<136;++steps){
        const auto p=host.nextMuteLedPacket(cursor);if(!p.valid)break;
        const bool fail=static_cast<int>(p.target)==failTarget &&
                        static_cast<int>(p.color)==failColor && p.on==failOn;
        if(!fail)k2.accept(p);else ++k2.failures;
        host.ackMuteLedPacket(p,!fail);
        if(fail)cursor=p.target+1;
    }
    assert(steps<136);
}
static Result cc(HostEngine& host,int number,int raw,int ms){
    const uint8_t midi[]={0xBE,static_cast<uint8_t>(number),static_cast<uint8_t>(raw)};
    return host.receive(0,midi,3,static_cast<int64_t>(ms)*48,ms);
}
int main(){
    {
        MuteLedOutput output;K2 k2;MuteLedOutput::Image semantic{};
        semantic[16]={MuteLedColor::Red,false,true};
        semantic[17]={MuteLedColor::Green,false,true};
        semantic[18]={MuteLedColor::Yellow,true,true}; // unrelated blinking owner
        render(output,k2,semantic,0);
        k2.only(16,0);k2.only(17,2);k2.only(18,1);
        // Inverting three simultaneous LEDs does not alter semantic image.
        output.flash(16,MuteLedColor::Red,100,&semantic[16]);
        output.flash(17,MuteLedColor::Green,100,&semantic[17]);
        output.flash(19,MuteLedColor::Red,100,&semantic[19]);
        render(output,k2,semantic,100,17,2,false); // injected LED OFF failure
        k2.only(16,-1);k2.only(17,2);k2.only(19,0);
        assert(k2.failures==1);
        render(output,k2,semantic,101); // retry failed target, no new CC!
        k2.only(16,-1);k2.only(17,-1);k2.only(19,0);
        render(output,k2,semantic,149);
        k2.only(16,-1);k2.only(17,-1);k2.only(19,0);
        // A *different* semantic LED is allowed to change while CC pulse
        // runs. Expiry must restore current semantic state, not a snapshot.
        semantic[16]={MuteLedColor::Yellow,false,true};
        render(output,k2,semantic,150,16,1,true); // injected restore ON failure
        k2.only(16,-1);k2.only(17,2);k2.only(19,-1);
        assert(k2.failures==2);
        render(output,k2,semantic,151); // callback recovered
        k2.only(16,1);k2.only(17,2);k2.only(19,-1);
        render(output,k2,semantic,999);
        k2.only(16,1);k2.only(17,2);k2.only(19,-1);
        // STOP invalidates only transient animation, not semantic base.
        output.flash(16,MuteLedColor::Yellow,1000,&semantic[16]);
        render(output,k2,semantic,1000);k2.only(16,-1);
        output.cancelFlashes();
        render(output,k2,semantic,1001);k2.only(16,1);
    }
    {
        HostEngine host;K2 k2;
        hostRender(host,k2,0);
        // EQ CC4 is a real physical K2 control; the first message
        // produces a RED witness with no session fixture.
        auto r=cc(host,4,63,10);assert(r.emit&&r.api==907);host.sent(r,0); // EQ stays at its 63 neutral
        hostRender(host,k2,10);
        k2.only(16,0);
        hostRender(host,k2,59);k2.only(16,0);
        hostRender(host,k2,60,16,0,false); // failed expiry NoteOff
        k2.only(16,0);assert(k2.failures==1);
        hostRender(host,k2,61);k2.only(16,-1);
        hostRender(host,k2,300);k2.only(16,-1);
        host.stop(301*48);hostRender(host,k2,302);k2.only(16,-1);
        // A newly accepted CC after transport STOP should not leave
        // an old transient light stuck when rendering proceeds.
        r=cc(host,4,65,400);if(r.emit)host.sent(r,0);
        hostRender(host,k2,400);
        hostRender(host,k2,1000);k2.only(16,-1);
    }
    std::cout<<"PASS SOC-05 no LED freeze after 50ms, callback loss/retry, simultaneous overlays, idle renders, STOP and semantic changes\n";
}
