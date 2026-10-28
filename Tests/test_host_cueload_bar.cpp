#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace xone::host;
static xone::l01::NavImage selected(int root,bool high){
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto n=nav.active();n.at(291)=root;
    n.at(299+2*root)=high?0:1;
    n.at(300+2*root)=high?1:0;
    n.at(293+2*root)=high?1:4;
    n.at(294+2*root)=high?8:0;
    assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
    return n;
}
static GridBatch bar(){
    GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,4};return b;
}
static void edge(HostEngine& h,unsigned p,bool down,int ms) {
    int note=-1;
    for(int n=24;n<40;++n)
        if(xone::l01::SurfaceInput::physicalControl(n)==static_cast<int>(p))note=n;
    assert(note>=0);
    const std::uint8_t midi[]={static_cast<std::uint8_t>(down?0x9E:0x8E),
        static_cast<std::uint8_t>(note),static_cast<std::uint8_t>(down?127:0)};
    assert(!h.receive(0,midi,3,ms*48,ms).emit);
}
static void successAll(HostEngine& h,int ms=0){
    h.beginCueLoadRetryPass();
    for(int i=0;i<16;++i){
        const auto p=h.nextCueLoadPacket();
        if(!p.valid)break;
        assert(p.bytes[0]==0xB6+p.api/128);
        assert(p.bytes[1]==p.api%128&&p.bytes[2]==p.value);
        h.ackCueLoadPacket(p,true,ms*48);
    }
    assert(!h.nextCueLoadPacket().valid);
}
int main(){
    // Every real physical CUE/LOAD row and both group selections. No
    // synthetic note mapping or inferred missing destinations.
    for(int root=0;root<2;++root)for(int high=0;high<2;++high){
        xone::l01::NavS3 n;assert(n.initializeDefault());
        auto navImage=selected(root,high!=0);
        assert(n.stage(navImage)&&n.commit());
        const unsigned group=static_cast<unsigned>(2*root+high);
        const unsigned base=high?4:0;
        std::array<int,4> physical{};physical.fill(-1);
        unsigned found=0;
        for(unsigned p=4;p<20;++p){
            unsigned g=0,i=0;
            if(!CueLoadBar::derive(n,p,g,i))continue;
            assert(g==group&&i>=base&&i<base+4);
            assert(physical[i-base]==-1);
            physical[i-base]=static_cast<int>(p);++found;
        }
        assert(found==4);
        for(int k=0;k<4;++k){
            HostEngine h;assert(h.setNavForDiagnostic(navImage));
            edge(h,static_cast<unsigned>(physical[k]),true,10);
            assert(h.pendingCueLoad()==1);
            assert(!h.nextCueLoadPacket().valid); // not before BAR
            edge(h,static_cast<unsigned>(physical[k]),false,11);
            assert(!h.nextCueLoadPacket().valid);
            h.onMusicalGrid(bar());
            const auto q=h.nextCueLoadPacket();
            const int expected=(root?939:931)+static_cast<int>(base)+k;
            assert(q.valid&&q.api==expected&&q.value==127);
            successAll(h,20);
            assert(h.cueLoadSent(group)==static_cast<int>(base)+k);
        }
    }
    // Different CUE/LOAD groups remain independent, including when changing
    // NAV between captured downs before the same host BAR.
    {
        HostEngine e;assert(e.setNavForDiagnostic(selected(0,false)));
        // Find a physical CUE group0 note (index0).
        xone::l01::NavS3 n;assert(n.initializeDefault());
        assert(n.stage(selected(0,false))&&n.commit());
        unsigned physical=0;
        for(unsigned p=4;p<20;++p){unsigned g=0,i=0;if(CueLoadBar::derive(n,p,g,i)&&i==0){physical=p;break;}}
        assert(physical>=4);edge(e,physical,true,10);edge(e,physical,false,11);
        assert(e.pendingCueLoad()==1);
        e.onMusicalGrid(bar());successAll(e);
        assert(e.cueLoadSent(0)==0);
        // Re-pressing the already delivered choice cannot create a fake
        // duplicate CC on the following BAR.
        edge(e,physical,true,20);edge(e,physical,false,21);
        e.onMusicalGrid(bar());assert(!e.nextCueLoadPacket().valid);
    }
    // Failed callback doesn't publish a selection; retries remain bounded.
    {
        CueLoadBar c;xone::l01::NavS3 n;assert(n.initializeDefault());
        assert(n.stage(selected(1,true))&&n.commit());
        unsigned physical=0;
        for(unsigned p=4;p<20;++p){unsigned g=0,i=0;if(CueLoadBar::derive(n,p,g,i)&&i==4){physical=p;break;}}
        assert(physical>=4&&c.request(n,physical));c.onBar();
        auto p=c.next();assert(p.valid&&p.api==943&&p.value==127);
        assert(c.acknowledge(p,false));
        assert(c.sentSelection(3)==-1&&!c.next().valid);
        c.beginRetryPass();
        auto retry=c.next();assert(retry.valid&&retry.api==p.api);
        assert(c.acknowledge(retry,true)&&c.sentSelection(3)==4);
        // Switch group3 index4 -> 5: OFF0 before ON127.
        unsigned other=0;
        for(unsigned x=4;x<20;++x){unsigned g=0,i=0;if(CueLoadBar::derive(n,x,g,i)&&i==5){other=x;break;}}
        assert(c.request(n,other));c.onBar();
        auto off=c.next();assert(off.valid&&off.api==943&&off.value==0);
        assert(c.acknowledge(off,true));
        auto on=c.next();assert(on.valid&&on.api==944&&on.value==127);
        assert(c.acknowledge(on,true)&&c.sentSelection(3)==5);
        // A new request after BAR must wait for another authenticated BAR.
        assert(c.request(n,physical));assert(!c.next().valid);
        c.stop();assert(c.pendingActions()==0&&c.sentSelection(3)==5);
        c.onBar();assert(!c.next().valid);
    }
    // Four independent source groups can commit on the SAME BAR. Each
    // retains its captured owner despite navigation among the groups.
    {
        CueLoadBar c;
        for(int root=0;root<2;++root)for(int high=0;high<2;++high){
            xone::l01::NavS3 n;assert(n.initializeDefault());
            assert(n.stage(selected(root,high!=0))&&n.commit());
            unsigned p=0;
            const unsigned choice=high?7:3;
            for(unsigned physical=4;physical<20;++physical){
                unsigned g=0,i=0;
                if(CueLoadBar::derive(n,physical,g,i)&&i==choice){p=physical;break;}
            }
            assert(p>=4&&c.request(n,p));
        }
        assert(c.pendingActions()==4&&!c.next().valid);
        c.onBar();
        for(int g=0;g<4;++g){
            const auto p=c.next();
            assert(p.valid&&p.group==static_cast<unsigned>(g));
            const int wanted=(g<2?931:939)+(g%2?7:3);
            assert(p.api==wanted&&p.value==127);
            assert(c.acknowledge(p,true));
        }
        assert(c.pendingActions()==0);
        assert(c.sentSelection(0)==3&&c.sentSelection(1)==7&&
               c.sentSelection(2)==3&&c.sentSelection(3)==7);
    }
    // Failed OFF cannot be bypassed by the dependent new ON.
    {
        CueLoadBar c;xone::l01::NavS3 n;assert(n.initializeDefault());
        assert(n.stage(selected(0,false))&&n.commit());
        unsigned a=0,b=0;
        for(unsigned physical=4;physical<20;++physical){
            unsigned g=0,i=0;if(!CueLoadBar::derive(n,physical,g,i))continue;
            if(i==0)a=physical;
            if(i==1)b=physical;
        }
        assert(a>=4&&b>=4&&c.request(n,a));c.onBar();
        auto first=c.next();assert(first.valid&&c.acknowledge(first,true));
        assert(c.request(n,b));c.onBar();
        auto off=c.next();assert(off.valid&&off.value==0);
        assert(c.acknowledge(off,false)&&!c.next().valid);
        c.beginRetryPass();
        auto again=c.next();assert(again.valid&&again.value==0&&again.api==off.api);
        assert(c.acknowledge(again,true));
        auto on=c.next();assert(on.valid&&on.value==127&&on.api!=off.api);
        assert(c.acknowledge(on,true)&&c.sentSelection(0)==1);
    }
    // RESET or preset overlays are NOT CUE/LOAD sources.
    {
        auto n=selected(0,false);n.at(292)=5;n.contextMode=0;
        assert(xone::l01::NavS3::regenerate(n)&&xone::l01::NavS3::validate(n));
        xone::l01::NavS3 nav;assert(nav.initializeDefault());assert(nav.stage(n)&&nav.commit());
        for(unsigned p=4;p<20;++p){unsigned g=0,i=0;assert(!CueLoadBar::derive(nav,p,g,i));}
    }
    std::cout<<"PASS source CUE/LOAD 16 physical mappings, BAR-only CC931..946, OFF/ON and callback retry\n";
}
