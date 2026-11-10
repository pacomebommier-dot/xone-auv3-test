// SOC-06: MACHINE TOP1..3 SHORT recall / LONG SAVE, BOT3 SMART restored.
// First eight MACHINES share source PATCH60, others have S8 PATCH30.
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace xone::host;
static void press(HostEngine& e,int note,bool down,int ms){
    const std::uint8_t midi[]{static_cast<std::uint8_t>(down?0x9E:0x8E),
        static_cast<std::uint8_t>(note),static_cast<std::uint8_t>(down?127:0)};
    assert(!e.receive(0,midi,3,ms*48,ms).emit);
}
static void top(HostEngine& e,int slot,int down,int up){
    press(e,52+slot,true,down);press(e,52+slot,false,up);
}
static void setup(HostEngine& e,bool complete,int selected=10){
    SessionImage image{};image.schema=1;image.config=5;image.revision=1;image.desired.fill(-2);
    const int base=320+42*(selected-1);
    for(int i=0;i<30;++i)image.desired[base+i]=i<18?20+i:i%2;
    if(!complete)image.desired[base+2]=-2;
    assert(e.restoreSessionBeforeRendering(image));
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto n=nav.active();n.at(292)=7;n.contextMode=7;n.at(314)=selected;
    assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
    assert(e.setNavForDiagnostic(n));
}
static GridBatch bar(int ordinal){
    GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,ordinal};return b;
}
int main(){
    for(bool initialLong:{false,true}){
        HostEngine e;setup(e,true);
        assert(e.setS8MachineModeBeforeRendering(9,0,4));
        assert(e.setS8MachineModeBeforeRendering(9,11,2));
        top(e,0,10,initialLong?210:11);
        assert(e.navMode()==7&&e.sampleSavesAccepted()==1&&e.sampleSavesRefused()==0);
        S8MachinePatchBank::Patch p{};
        assert(e.s8MachinePatchForDiagnostic(9,0,p)&&p.valid&&p.config==5&&p.revision==1);
        for(int i=0;i<30;++i)assert(p.values[i]==(i<18?20+i:i%2));
        assert(e.s8MachineMode(9,0)==4&&e.s8MachineMode(9,11)==2);
        top(e,0,250,450); // exactly 200 = overwrite SAVE
        assert(e.sampleSavesAccepted()==2&&!e.samplePatchPending());
        assert(e.s8MachinePatchForDiagnostic(9,0,p)&&p.revision==2);
        top(e,0,500,501);assert(e.sampleSavesAccepted()==2&&e.samplePatchPending());
        const auto result=e.samplePatchOnGrid(bar(4),4*48*100);
        assert(result.packetCount()==30&&!e.samplePatchPending());
        for(unsigned n=0;n<result.packetCount();++n){
            const auto packet=result.packet(n);assert(packet.musical);
            const int api=n<18?128+18*9+static_cast<int>(n):96+12*9+static_cast<int>(n)-18;
            assert(packet.api==api&&packet.type==(n<18?10:11));
            assert(e.canSend(packet));e.sent(packet,0);
        }
        assert(e.s8MachineMode(9,0)==4&&e.s8MachineMode(9,11)==2);
        for(int i=30;i<42;++i)assert(!e.desiredLive(698+i).known);
    }
    // UNKNOWN first SAVE refuses atomically, with no fallback recall.
    {
        HostEngine e;setup(e,false);top(e,0,10,210);
        assert(e.sampleSavesRefused()==1&&e.sampleSavesAccepted()==0&&!e.samplePatchPending());
        S8MachinePatchBank::Patch p{};
        assert(e.s8MachinePatchForDiagnostic(9,0,p)&&!p.valid);
    }
    // BOT3 short from MACHINE = persistent GLOBAL; BOT3 LONG = temporary GLOBAL -> MACHINE.
    for(bool timer:{false,true}){
        HostEngine e;setup(e,true);
        press(e,14,true,10);
        if(timer){e.advance(210,210*48);assert(e.navMode()==2&&e.longNavPending());}
        press(e,14,false,210);
        assert(e.navMode()==7&&!e.longNavPending());
        press(e,14,true,240);press(e,14,false,241);
        assert(e.navMode()==2);
    }
    // Old BOT3+TOP is NOT a SAVE chord: a filled short TOP recalls.
    {
        HostEngine e;setup(e,true);top(e,0,10,11);assert(e.sampleSavesAccepted()==1);
        press(e,14,true,20);top(e,0,30,31);press(e,14,false,40);
        assert(e.sampleSavesAccepted()==1&&e.samplePatchPending()&&e.navMode()==2);
    }
    // Three slots per each of 24 MACHINES; only initial validity is false.
    {
        S8MachinePatchBank b;S8MachinePatchBank::Patch p{};
        S8MachinePatchBank::Values values{};values.fill(42);
        for(int i=18;i<30;++i)values[i]=i%2;
        for(unsigned machine=0;machine<24;++machine)for(unsigned slot=0;slot<3;++slot){
            assert(b.read(machine,slot,p)&&!p.valid);
            assert(b.save(machine,slot,1,values,0));
            assert(b.read(machine,slot,p)&&p.valid&&p.revision==1);
        }
        assert(b.setMode(23,11,3)&&b.mode(23,11)==3);
        assert(!b.save(24,0,1,values,0)&&!b.save(0,3,1,values,0));
        assert(!b.save(23,2,1,values,0));
        assert(b.mode(0,11)==-2);
    }
    std::cout<<"PASS SOC-06 MACHINE TOP short/long, EMPTY, BAR, BOT3 momentary, UNKNOWN\n";
}
