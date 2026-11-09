// SOC-02: source-qualified effect SPOT group and steady state LEDs in SOCLE.
// Borrowed RESET blinking belongs to SOC-03; CC pickup flash belongs SOC-05.
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static Result key(HostEngine& e,int note,bool down,int ms) {
    const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),
                      static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};
    return e.receive(0,b,3,ms,ms);
}
static void selectInst1(HostEngine& e){key(e,36,true,0);key(e,36,false,10);assert(e.navMode()==0);}
static int tx(HostEngine& e,int expectValue) {
    int count=0;
    for(int i=0;i<8;++i){
        auto p=e.nextSourcePacket();if(!p.valid)break;
        assert(p.type==11 && p.value==expectValue);
        assert(p.bytes[2]==(expectValue?127:0));
        e.ackSourcePacket(p,true,0);++count;
    }
    return count;
}
static void check(HostEngine& e,int a,int b,int c){
    const auto live=e.documentForHost().live;
    const int values[]{a,b,c};
    for(int k=0;k<3;++k){const int saved=live.desired[1768+k];
        assert(saved==values[k]||(saved==-2&&values[k]==0));}

    const auto image=e.ledImage();
    const int positions[]{16,20,24};
    for(int k=0;k<3;++k){
        auto led=image[static_cast<unsigned>(positions[k])];
        assert(!led.blinking); // stable state, not a borrowed RESET flash
        const int saved=live.desired[1768+k];
        if(saved==-2)assert(values[k]==0&&!led.known);
        else assert(led.known&&led.color==(values[k]?MuteLedColor::Red:MuteLedColor::Off));
    }
}
int main(){
    HostEngine e;
    // Missing LIVE states remain UNKNOWN in documents, but the SOCLE SPOT
    // default-OFF resolves for input, while UNKNOWN LEDs remain dark before a real write.
    assert(!e.desiredLive(1768).known);
    selectInst1(e);check(e,0,0,0);
    auto r=key(e,48,true,20);assert(r.emit&&r.api==384);e.sent(r,0);
    r=key(e,48,false,70);assert(!r.emit);check(e,1,0,0); // short: only touched
    r=key(e,48,true,100);assert(!r.emit);
    e.advance(220,220); // >=120ms: initial ON -> whole group OFF
    assert(tx(e,0)==1);check(e,0,0,0);
    assert(!key(e,48,false,250).emit);check(e,0,0,0);
    r=key(e,48,true,300);assert(r.emit);e.sent(r,0);
    e.advance(420,420); // initial OFF -> whole group ON
    assert(tx(e,1)==2);check(e,1,1,1);
    assert(!key(e,48,false,450).emit);check(e,1,1,1);
    // Toggle one member OFF (mixed group), then long from OFF to all ON.
    r=key(e,44,true,500);assert(!r.emit);
    r=key(e,44,false,550);assert(r.emit&&r.api==385);e.sent(r,0);
    check(e,1,0,1);
    r=key(e,44,true,600);assert(r.emit&&r.api==385);e.sent(r,0);
    e.advance(720,720);assert(tx(e,1)==0); // touched DOWN already sent ON
    check(e,1,1,1);
    assert(!key(e,44,false,740).emit);
    // 119ms must remain individual; STOP keeps owed NoteOff management.
    r=key(e,48,true,800);assert(!r.emit);
    e.advance(919,919);assert(!e.nextSourcePacket().valid);
    r=key(e,48,false,919);assert(r.emit&&r.api==384);e.sent(r,0);
    check(e,0,1,1);
    // Exact threshold on physical UP: no timer tick is required.
    HostEngine boundary;selectInst1(boundary);
    r=key(boundary,48,true,20);assert(r.emit);boundary.sent(r,0);
    assert(!key(boundary,48,false,140).emit);
    assert(tx(boundary,1)==2);check(boundary,1,1,1);
    // Physical LED projection: three red SPOT LEDs turn ON, no blinking.
    boundary.beginRenderBuffer();boundary.prepareMuteLeds(140);
    unsigned cursor=0,lit=0;
    for(unsigned k=0;k<136;++k){
        auto p=boundary.nextMuteLedPacket(cursor);if(!p.valid)break;
        if(p.on&&(p.target==16||p.target==20||p.target==24)){
            assert(p.color==0);++lit;
        }
        boundary.ackMuteLedPacket(p,true);
    }
    assert(lit==3);
    r=key(boundary,48,true,200);assert(!r.emit);
    r=key(boundary,48,false,319);assert(r.emit);boundary.sent(r,0);
    assert(tx(boundary,0)==0);check(boundary,0,1,1);
    boundary.beginRenderBuffer();boundary.prepareMuteLeds(319);
    cursor=0;unsigned dark=0;
    for(unsigned k=0;k<136;++k){
        auto p=boundary.nextMuteLedPacket(cursor);if(!p.valid)break;
        if(!p.on&&p.target==16&&p.color==0)++dark;
        boundary.ackMuteLedPacket(p,true);
    }
    assert(dark==1);
    std::cout<<"PASS SOC-02 INST1 SPOT short, 120ms group from ON/OFF/mixed, release and LED state\n";
}
