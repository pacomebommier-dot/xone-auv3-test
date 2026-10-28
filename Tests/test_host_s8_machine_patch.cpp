#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace xone::host;
static void press(HostEngine& e,int note,bool down,int ms) {
    const std::uint8_t midi[]{static_cast<std::uint8_t>(down?0x9E:0x8E),
        static_cast<std::uint8_t>(note),static_cast<std::uint8_t>(down?127:0)};
    assert(!e.receive(0,midi,3,ms*48,ms).emit);
}
static void setup(HostEngine& e,bool complete,int selected=2) {
    xone::l01::LiveState::SessionImage image{};
    image.schema=1;image.config=5;image.revision=1;image.desired.fill(-2);
    const int base=320+42*(selected-1);
    for(int i=0;i<30;++i)image.desired[base+i]=i<18?20+i:i%2;
    if(!complete)image.desired[base+2]=-2;
    assert(e.restoreSessionBeforeRendering(image));
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto n=nav.active();n.at(292)=7;n.contextMode=7;n.at(314)=selected;
    assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
    assert(e.setNavForDiagnostic(n));
}
static GridBatch bar(int n) {
    GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,n};return b;
}
int main(){
    // Real BOT3+TOP1 (no 90/200ms cap): SAVE to MACHINE2 slot1.
    for(int delay:{30,150,800}){
        HostEngine e;setup(e,true);
        assert(e.setS8MachineModeBeforeRendering(1,0,4));
        assert(e.setS8MachineModeBeforeRendering(1,11,2));
        press(e,14,true,10);
        if(delay>200)e.advance(delay+9,(delay+9)*48);
        assert(e.navMode()==7);
        press(e,52,true,10+delay);
        press(e,52,false,11+delay);
        press(e,14,false,12+delay);
        assert(e.navMode()==7&&e.sampleSavesAccepted()==1&&e.sampleSavesRefused()==0);
        S8MachinePatchBank::Patch p{};
        assert(e.s8MachinePatchForDiagnostic(1,0,p)&&p.valid&&p.config==5&&p.revision==1);
        for(int i=0;i<30;++i)assert(p.values[i]==(i<18?20+i:i%2));
        assert(e.s8MachineMode(1,0)==4&&e.s8MachineMode(1,11)==2);
        assert(!e.samplePatchPending()); // SAVE never simultaneously RECALL
        if(delay==30){
            press(e,52,true,80);press(e,52,false,81);
            assert(e.samplePatchPending());
            const auto midi=e.samplePatchOnGrid(bar(4),4*48*100);
            assert(midi.packetCount()==30&&!e.samplePatchPending());
            for(unsigned n=0;n<midi.packetCount();++n){
                const auto p=midi.packet(n);assert(p.musical);
                const int api=n<18?128+18+static_cast<int>(n):96+12+static_cast<int>(n)-18;
                assert(p.api==api&&p.type==(n<18?10:11));
                assert(e.canSend(p));e.sent(p,0);
            }
            assert(e.s8MachineMode(1,0)==4&&e.s8MachineMode(1,11)==2);
            for(int i=30;i<42;++i)assert(!e.desiredLive(362+i).known);
        }
    }
    // Unknown instrument parameter refuses SAVE without creating a patch.
    {
        HostEngine e;setup(e,false);
        press(e,14,true,10);press(e,52,true,40);press(e,52,false,41);press(e,14,false,42);
        assert(e.sampleSavesRefused()==1&&e.sampleSavesAccepted()==0);
        S8MachinePatchBank::Patch p{};
        assert(e.s8MachinePatchForDiagnostic(1,0,p)&&!p.valid);
        HostEngine::TopIntent t{};assert(!e.lastTopIntent(0,t));
    }
    // All 24 machine selectors and slots are distinct; none starts initialized.
    {
        S8MachinePatchBank b;S8MachinePatchBank::Patch p{};
        S8MachinePatchBank::Values values{};values.fill(42);
        for(int i=18;i<30;++i)values[i]=i%2;
        for(unsigned machine=0;machine<24;++machine){
            for(unsigned slot=0;slot<3;++slot){
                assert(b.read(machine,slot,p)&&!p.valid);
                assert(b.save(machine,slot,1,values,0));
                assert(b.read(machine,slot,p)&&p.valid&&p.revision==1);
            }
        }
        assert(b.setMode(23,11,3)&&b.mode(23,11)==3);
        assert(!b.save(24,0,1,values,0)&&!b.save(0,3,1,values,0));
        assert(!b.save(23,2,1,values,0));
        assert(b.mode(0,11)==-2);
    }
    std::cout<<"PASS S8 MACHINE BOT3+TOP SAVE30, late hold 800ms, BAR recall 18 CC/12 NOTE, mode exclusion\n";
}
