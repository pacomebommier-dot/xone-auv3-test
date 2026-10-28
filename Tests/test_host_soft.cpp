#include "../Shared/XoneSoftMidiBridge.hpp"
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static GridBatch edge(GridKind kind,std::int64_t ordinal) {
    GridBatch g{};g.valid=true;g.count=1;g.events[0]={kind,0,ordinal};return g;
}
static void take(SoftMidiBridge& s,unsigned index,bool on,bool succeed=true) {
    unsigned cursor=0;auto p=s.next(cursor);
    assert(p.valid&&p.index==index&&p.on==on);
    const unsigned a=544+index;
    assert(p.bytes[0]==(on?0x90:0x80)+6+a/128);
    assert(p.bytes[1]==a%128&&p.bytes[2]==(on?127:0));
    s.confirm(p,succeed);
}
int main(){
    SoftMidiBridge s;
    assert(!s.requestVerified(2,0,0)&&!s.requestVerified(0,8,0)&&!s.requestVerified(0,0,4));
    assert(s.requestVerified(0,0,0));assert(s.requestVerified(0,0,0)); // pending coalesces
    assert(s.pendingCount()==1&&s.active(0,0)==-1);
    assert(s.observe(edge(GridKind::Beat,1))==0);
    GridBatch unknown{};assert(s.observe(unknown)==0&&s.pendingCount()==1);
    assert(s.observe(edge(GridKind::Bar,2))==1&&s.pendingCount()==0&&s.active(0,0)==0);
    take(s,0,true,false); // callback error does not claim transmission
    take(s,0,true,true);
    unsigned cursor=0;assert(!s.next(cursor).valid);
    // Re-selecting a valid choice arms one OFF->ON, not a silent no-op.
    assert(s.requestVerified(0,0,0));
    assert(s.observe(edge(GridKind::Bar,3))==1);
    take(s,0,false,false); // OFF failure blocks a dependent ON
    cursor=1;assert(!s.next(cursor).valid);
    take(s,0,false,true);take(s,0,true,true);
    // One page does not disturb another family/page.
    assert(s.requestVerified(1,7,3));
    assert(s.observe(edge(GridKind::Bar,4))==1);
    take(s,63,true,true);assert(s.active(1,7)==3&&s.active(0,0)==0);
    assert(s.requestVerified(0,0,1));
    assert(s.observe(edge(GridKind::Bar,5))==1);
    take(s,0,false,true);take(s,1,true,true);
    assert(s.active(0,0)==1);
    assert(s.requestVerified(0,0,2));s.stop();
    assert(s.pendingCount()==0&&s.active(0,0)==1);
    assert(s.observe(edge(GridKind::Bar,6))==0);
    // S3's verified route formula, not a guessed synthetic MIDI channel:
    xone::l01::NavS3 nav;assert(nav.initializeDefault());
    auto page=nav.active();page.at(294)=6;page.at(300)=1;
    assert(xone::l01::NavS3::regenerate(page)&&xone::l01::NavS3::validate(page));
    assert(nav.stage(page)&&nav.commit());
    const auto drum=nav.active().route(4);
    assert(drum.address/1024==5&&drum.position==0);
    SoftMidiBridge fromRoute;
    assert(fromRoute.requestFromRoute(4,drum,0));
    assert(!fromRoute.requestFromRoute(8,drum,0)); // owner/choice mismatch
    assert(fromRoute.observe(edge(GridKind::Bar,7))==1&&fromRoute.active(0,1)==0);
    take(fromRoute,4,true,true);
    page=nav.active();page.at(300)=0;page.at(297)=3;page.at(303)=1;
    assert(xone::l01::NavS3::regenerate(page)&&xone::l01::NavS3::validate(page));
    assert(nav.stage(page)&&nav.commit());
    const auto fill=nav.active().route(4);
    assert(fill.address/1024==8&&fill.position==0);
    assert(fromRoute.requestFromRoute(4,fill,0));
    assert(fromRoute.observe(edge(GridKind::Bar,8))==1&&fromRoute.active(1,2)==0);
    take(fromRoute,40,true,true); // MIDI11 note 72
    // Full receive path: K2 DOWN, no pre-BAR MIDI, exact SOFT after BAR.
    HostEngine host;assert(host.setNavForDiagnostic(page));
    const uint8_t down[]{0x9E,36,127},up[]{0x8E,36,0};
    assert(!host.receive(0,down,3,0,0).emit);
    assert(host.pendingSoftCount()==1);
    assert(!host.receive(0,down,3,1,1).emit&&host.pendingSoftCount()==1);
    assert(!host.receive(0,up,3,2,2).emit);
    assert(host.onMusicalGrid(edge(GridKind::Beat,1))==0);
    assert(host.appliedSoftChoice(1,2)==-1);
    assert(host.onMusicalGrid(edge(GridKind::Bar,2))==1);
    assert(host.appliedSoftChoice(1,2)==0);
    cursor=0;auto p=host.nextSoftPacket(cursor);
    assert(p.valid&&p.index==40&&p.bytes[0]==0x9A&&p.bytes[1]==72&&p.bytes[2]==127);
    host.ackSoftPacket(p,true);cursor=0;assert(!host.nextSoftPacket(cursor).valid);
    // STOP cancels only pending; no fabricated BAR, UP or extra ON.
    assert(!host.receive(0,down,3,3,3).emit);
    assert(host.pendingSoftCount()==1);host.stop(4);
    assert(host.pendingSoftCount()==0&&host.appliedSoftChoice(1,2)==0);
    // STOP owes an OFF for every possibly delivered ON, at the captured note.
    cursor=0;p=host.nextSoftPacket(cursor);
    assert(p.valid&&!p.on&&p.index==40&&p.bytes[0]==0x8A&&p.bytes[1]==72);
    host.ackSoftPacket(p,false); // failed STOP OFF is retried, not forgotten
    cursor=0;p=host.nextSoftPacket(cursor);assert(p.valid&&!p.on&&p.index==40);
    host.ackSoftPacket(p,true);
    cursor=0;assert(!host.nextSoftPacket(cursor).valid);
    assert(host.onMusicalGrid(edge(GridKind::Bar,3))==0);
    cursor=0;assert(!host.nextSoftPacket(cursor).valid); // no historical ON replay
    std::cout<<"PASS S7 SOFT 16x4 physical S3 route, BAR commit, OFF-before-ON, callback retry, STOP\n";
}
