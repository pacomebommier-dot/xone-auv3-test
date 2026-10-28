#include "../Shared/XoneHostEngine.hpp"
#include "../Shared/XoneValidationFixture.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static xone::l01::NavImage mode(int m){
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto n=nav.active();n.at(292)=m;n.contextMode=m;
    assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
    return n;
}
static void note(HostEngine& e,int n,bool down,int ms){
    const std::uint8_t msg[]{static_cast<std::uint8_t>(down?0x9E:0x8E),
        static_cast<std::uint8_t>(n),static_cast<std::uint8_t>(down?127:0)};
    assert(!e.receive(0,msg,3,48*ms,ms).emit);
}
static void configured(HostEngine& e,bool complete){
    xone::host::SessionImage image{};
    image.schema=1;image.config=1;image.revision=1;image.desired.fill(-2);
    for(int i=0;i<40;++i)image.desired[i]=i>=28?1:(i>=16?(i%2):(i+17)%128);
    if(!complete)image.desired[4]=-2;
    assert(e.restoreSessionBeforeRendering(image));
    assert(e.setNavForDiagnostic(mode(3)));
}
static void save(HostEngine& e,int botDownMs,int topDownMs,int slot){
    const int top=52+slot;
    note(e,13,true,botDownMs);
    if(topDownMs-botDownMs>200)e.advance(topDownMs-1,(topDownMs-1)*48);
    assert(e.navMode()==3); // no prematurity LONG NAV
    note(e,top,true,topDownMs);
    note(e,top,false,topDownMs+1);
    note(e,13,false,topDownMs+2);
    assert(e.navMode()==3); // consumed BOT cannot open MUTE/PRESET/GLOBAL
}
int main(){
    for(int gap:{30,150,800}){
        HostEngine e;configured(e,true);
        e.enableNativeMutePage(true);
        save(e,10,10+gap,0);
        assert(e.sampleSavesAccepted()==1&&e.sampleSavesRefused()==0);
        assert(!e.mutePageVisible()&&!e.samplePatchPending());
        HostEngine::TopIntent unintended{};
        assert(!e.lastTopIntent(0,unintended)); // saved TOP must not arm recall
        xone::l01::BankS6::PatchRecord record{};
        assert(e.samplePatchForDiagnostic(0,0,record)&&record.valid);
        assert(record.config==1&&record.revision==1);
        for(int i=0;i<40;++i)assert(record.values[i]==(i>=28?1:(i>=16?(i%2):(i+17)%128)));
        for(int i=40;i<84;++i)assert(record.values[i]==-2);
    }
    // ALL three physical PATCH slots have distinct storage.
    {
        HostEngine e;configured(e,true);
        for(int slot=0;slot<3;++slot)save(e,slot*50,slot*50+30,slot);
        assert(e.sampleSavesAccepted()==3);
        for(int slot=0;slot<3;++slot){
            xone::l01::BankS6::PatchRecord p{};
            assert(e.samplePatchForDiagnostic(0,slot,p)&&p.valid&&p.revision==1);
        }
    }
    // Unknown LIVE fails atomically: no fabricated default, no TOP RECALL.
    {
        HostEngine e;configured(e,false);
        save(e,10,40,1);
        assert(e.sampleSavesRefused()==1&&e.sampleSavesAccepted()==0);
        xone::l01::BankS6::PatchRecord record{};
        assert(!e.samplePatchForDiagnostic(0,1,record));
        HostEngine::TopIntent intent{};assert(!e.lastTopIntent(1,intent));
    }
    // No TOP: BOT2 remains local MUTE entry (persistent even after 800ms).
    {
        HostEngine e;configured(e,true);e.enableNativeMutePage(true);
        note(e,13,true,10);e.advance(810,810*48);
        assert(e.navMode()==3&&!e.mutePageVisible());
        note(e,13,false,811);
        assert(e.mutePageVisible()&&e.navMode()==3);
    }
    // TOP first, BOT after: NEVER retroactive SAVE; existing TOP recall only.
    {
        HostEngine e;configured(e,true);
        note(e,52,true,10);note(e,13,true,20);
        note(e,52,false,30);note(e,13,false,40);
        assert(e.sampleSavesAccepted()==0);
        HostEngine::TopIntent intent{};assert(e.lastTopIntent(0,intent));
    }
    // Other BOT in <90ms forms combo and consumes the candidate, not SAVE.
    {
        HostEngine e;configured(e,true);
        note(e,13,true,10);note(e,14,true,40);note(e,52,true,50);
        assert(e.sampleSavesAccepted()==0);
        note(e,52,false,60);note(e,14,false,61);note(e,13,false,62);
    }
    // MACHINE SAVE is explicitly blocked until S8 modes-excluding adapter.
    // Consumed TOP still must not fall through into RECALL or GLOBAL NAV.
    {
        HostEngine e;assert(e.setNavForDiagnostic(mode(7)));
        note(e,14,true,10);e.advance(810,810*48);assert(e.navMode()==7);
        note(e,52,true,820);note(e,52,false,821);note(e,14,false,822);
        assert(e.sampleSavesAccepted()==0&&e.sampleSavesRefused()==1);
        assert(e.navMode()==7&&!e.samplePatchPending());
        HostEngine::TopIntent i{};assert(!e.lastTopIntent(0,i));
    }
    // MACHINE BOT3 with no TOP -> GLOBAL, persistently, not LONG/return.
    {
        HostEngine e;assert(e.setNavForDiagnostic(mode(7)));
        note(e,14,true,10);e.advance(810,810*48);assert(e.navMode()==7);
        note(e,14,false,811);assert(e.navMode()==2);
    }
    // STOP invalidates the local candidate; a later TOP cannot use an old BOT.
    {
        HostEngine e;configured(e,true);
        note(e,13,true,10);e.stop(11*48);note(e,52,true,20);
        note(e,52,false,21);note(e,13,false,22);
        assert(e.sampleSavesAccepted()==0);
    }
    std::cout<<"PASS BOT2+TOP1..3 SAMPLE SAVE40, 30/150/800ms, UNKNOWN rejection, combos, STOP, MACHINE refusal\n";
}
