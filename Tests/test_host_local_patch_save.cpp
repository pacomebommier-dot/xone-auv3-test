// SOC-06: no BOT+TOP SAVE; SAMPLE TOP SHORT/long decides at true UP.
// BOT2 remains global MUTE SMART, including when SAMPLE is visible.
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace xone::host;
static xone::l01::NavImage sampleNav(){
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto n=nav.active();n.at(292)=3;n.contextMode=3;
    assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
    return n;
}
static void note(HostEngine& e,int n,bool down,int ms){
    const std::uint8_t msg[]{static_cast<std::uint8_t>(down?0x9E:0x8E),
        static_cast<std::uint8_t>(n),static_cast<std::uint8_t>(down?127:0)};
    assert(!e.receive(0,msg,3,48*ms,ms).emit);
}
static void configured(HostEngine& e,bool complete){
    SessionImage image{};image.schema=1;image.config=1;image.revision=1;image.desired.fill(-2);
    for(int i=0;i<40;++i)image.desired[i]=i>=28?1:(i>=16?(i%2):(i+17)%128);
    if(!complete)image.desired[4]=-2;
    assert(e.restoreSessionBeforeRendering(image));
    assert(e.setNavForDiagnostic(sampleNav()));
}
static void top(HostEngine& e,int slot,int down,int up){
    note(e,52+slot,true,down);note(e,52+slot,false,up);
}
int main(){
    // Empty slot SHORT or LONG captures once; filled LONG overwrites.
    for(bool initialLong:{false,true}){
        HostEngine e;configured(e,true);e.enableNativeMutePage(true);
        assert(e.setS8SampleModeBeforeRendering(0,0,4));
        assert(e.setS8SampleModeBeforeRendering(0,11,2));
        top(e,0,10,initialLong?210:11);
        assert(e.sampleSavesAccepted()==1&&e.sampleSavesRefused()==0&&!e.samplePatchPending());
        S8SamplePatchBank::Patch p{};
        assert(e.s8SamplePatchForDiagnostic(0,0,p)&&p.valid&&p.revision==1);
        assert(p.values[0]==17&&e.s8SampleMode(0,0)==4&&e.s8SampleMode(0,11)==2);
        top(e,0,250,450); // 200ms exactly = LONG, no prematurity
        assert(e.sampleSavesAccepted()==2&&!e.samplePatchPending());
        assert(e.s8SamplePatchForDiagnostic(0,0,p)&&p.revision==2);
        top(e,0,500,501); // SHORT = BAR recall, not SAVE
        assert(e.sampleSavesAccepted()==2&&e.samplePatchPending());
        GridBatch beat{};beat.valid=true;beat.count=1;beat.events[0]={GridKind::Beat,0,1};
        assert(!e.samplePatchOnGrid(beat,1000).emit&&e.samplePatchPending());
        GridBatch bar{};bar.valid=true;bar.count=1;bar.events[0]={GridKind::Bar,0,4};
        const auto result=e.samplePatchOnGrid(bar,2000);
        assert(result.packetCount()==28&&!e.samplePatchPending());
        for(unsigned j=0;j<result.packetCount();++j){
            const auto packet=result.packet(j);assert(packet.musical&&e.canSend(packet));e.sent(packet,0);
        }
        for(int i=28;i<40;++i)assert(e.desiredLive(i).value==1); // modes untouched
    }
    // Three independent physical slots, no BOT needed.
    {
        HostEngine e;configured(e,true);
        for(int slot=0;slot<3;++slot)top(e,slot,10+20*slot,11+20*slot);
        assert(e.sampleSavesAccepted()==3);
        for(int slot=0;slot<3;++slot){
            S8SamplePatchBank::Patch p{};
            assert(e.s8SamplePatchForDiagnostic(0,slot,p)&&p.valid&&p.revision==1);
        }
    }
    // UNKNOWN LIVE: refused first SAVE must not create or recall anything.
    {
        HostEngine e;configured(e,false);top(e,1,10,210);
        assert(e.sampleSavesRefused()==1&&e.sampleSavesAccepted()==0&&!e.samplePatchPending());
        S8SamplePatchBank::Patch p{};
        assert(e.s8SamplePatchForDiagnostic(0,1,p)&&!p.valid);
    }
    // BOT2 LONG from SAMPLE is truly momentary; no special persistent-UP exit.
    for(bool timer:{false,true}){
        HostEngine e;configured(e,true);e.enableNativeMutePage(true);
        note(e,13,true,10);
        if(timer){e.advance(210,210*48);assert(e.mutePageVisible()&&e.navMode()==3);}
        note(e,13,false,210);
        assert(!e.mutePageVisible()&&e.navMode()==3);
        note(e,13,true,240);note(e,13,false,241);
        assert(e.mutePageVisible()); // SHORT remains persistent
        note(e,13,true,260);note(e,13,false,261);
        assert(!e.mutePageVisible());
    }
    // Old BOT2+TOP sequence no longer triggers SAVE on a filled slot.
    {
        HostEngine e;configured(e,true);e.enableNativeMutePage(true);
        top(e,0,10,11);assert(e.sampleSavesAccepted()==1);
        note(e,13,true,20);top(e,0,30,31);note(e,13,false,40);
        assert(e.sampleSavesAccepted()==1&&e.samplePatchPending());
    }
    // STOP cancels a TOP hold; no phantom capture at late UP.
    {
        HostEngine e;configured(e,true);note(e,52,true,10);
        e.stop(11*48);note(e,52,false,210);
        assert(e.sampleSavesAccepted()==0);
        S8SamplePatchBank::Patch p{};assert(e.s8SamplePatchForDiagnostic(0,0,p)&&!p.valid);
    }
    std::cout<<"PASS SOC-06 SAMPLE TOP short/long, EMPTY, recall BAR, BOT2 momentary, UNKNOWN, STOP\n";
}
