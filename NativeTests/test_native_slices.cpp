#include "../Core/NativeSlices.hpp"
#include "../Core/Engine.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>
using namespace xone;
using namespace xone::l01;
namespace {
unsigned verified=0;
void check(bool ok,const char* label){if(!ok)throw std::runtime_error(label);}
template<typename Fn>void scenario(const char* id, Fn&& f){f();++verified;std::cout<<"PASS "<<id<<'\n';}
std::array<int,11> frame(int type,int control,int value){return {125,88,75,50,7,1,1,2,type,control,value};}
std::array<int,76> image(){std::array<int,76> a{};a[0]=125;a[1]=88;a[2]=75;a[3]=50;a[4]=7;a[5]=9;a[6]=3;a[7]=7;return a;}
}
int main(){
try{
scenario("IN-CHANNEL",[]{SurfaceInput in;auto e=in.accept({13,0x90,12,127});check(e.kind==SurfaceKind::None,"wrong ch");check(!in.held(0),"wrong ch held");});
scenario("IN-MAP-36",[]{
 const std::array<std::array<int,2>,36> expect{{
 {{12,0}},{{13,1}},{{14,2}},{{15,3}},{{24,16}},{{25,17}},{{26,18}},{{27,19}},
 {{28,12}},{{29,13}},{{30,14}},{{31,15}},{{32,8}},{{33,9}},{{34,10}},{{35,11}},
 {{36,4}},{{37,5}},{{38,6}},{{39,7}},{{40,28}},{{41,29}},{{42,30}},{{43,31}},
 {{44,24}},{{45,25}},{{46,26}},{{47,27}},{{48,20}},{{49,21}},{{50,22}},{{51,23}},
 {{52,32}},{{53,33}},{{54,34}},{{55,35}} }};
 SurfaceInput in;std::array<bool,36> covered{};
 for(const auto& pair:expect){const int note=pair[0],control=pair[1];check(SurfaceInput::physicalControl(note)==control,"map mismatch");auto v=in.accept({14,0x90,note,127});check(v.kind==SurfaceKind::Edge&&v.control==control&&v.phase==1,"on mismatch");check(!covered[static_cast<std::size_t>(control)],"duplicate mapping");covered[static_cast<std::size_t>(control)]=true;}
 for(bool b:covered)check(b,"missing control");
 for(int i=0;i<128;++i){bool mapped=false;for(const auto& p:expect)if(p[0]==i)mapped=true;if(!mapped)check(SurfaceInput::physicalControl(i)==-1,"stray mapping");}
 });
scenario("IN-DUPLICATE",[]{SurfaceInput s;check(s.accept({14,0x90,12,127}).kind==SurfaceKind::Edge,"first down");check(s.accept({14,0x90,12,127}).kind==SurfaceKind::None,"duplicate down");});
scenario("IN-NOTEON0",[]{SurfaceInput s;s.accept({14,0x90,12,127});const auto a=s.accept({14,0x90,12,0});check(a.kind==SurfaceKind::Edge&&a.phase==0&&!s.held(0),"noteon vel0");});
scenario("IN-DJ",[]{SurfaceInput s;auto a=s.accept({14,0xB0,21,77});check(a.kind==SurfaceKind::Dj&&a.number==21&&a.value==77,"dj route");});
scenario("IN-CC-ORDER-S1",[]{SurfaceInput s;auto a=s.accept({14,0xB0,4,64});check(a.kind==SurfaceKind::Cc&&a.number==4&&a.value==64,"select CC");check(s.accept({14,0xB0,22,10}).kind==SurfaceKind::None,"no generic thru");});
scenario("IN-STOP-HELD",[]{SurfaceInput s;s.accept({14,0x90,12,127});s.onHostStop();check(s.held(0),"stop fabricated up");const auto a=s.accept({14,0x80,12,0});check(a.kind==SurfaceKind::Edge&&a.phase==0,"true up lost");});
scenario("IN-RELOAD-HELD",[]{SurfaceInput first;auto a=first.accept({14,0x90,12,100});check(a.kind==SurfaceKind::Edge,"down");SurfaceInput reloaded(first.heldState());check(reloaded.held(0),"held not preserved");check(reloaded.accept({14,0x90,12,100}).kind==SurfaceKind::None,"ghost second down");check(reloaded.accept({14,0x80,12,0}).phase==0&&!reloaded.held(0),"reload true up");});
scenario("GEST-FRAME",[]{auto f=frame(2,0,1);f[3]=9;check(!decodeS1Frame(f).valid,"bad header");f=frame(2,0,1);f[9]=36;check(!decodeS1Frame(f).valid,"bad control");f=frame(3,21,77);check(decodeS1Frame(f).valid,"valid DJ");});
scenario("GEST-ORPHAN-UP",[]{RawGestureIngress s;check(s.accept(decodeS1Frame(frame(2,0,0))),"must forward orphan raw UP");check(s.orphanUps()==1,"orphan count");});
scenario("GEST-DUPLICATE-DOWN",[]{RawGestureIngress s;auto v=decodeS1Frame(frame(2,0,1));check(s.accept(v),"first down");check(!s.accept(v),"duplicate forwarded");});
scenario("GEST-90-200",[]{BotClock c;check(c.down(1,1000),"down");check(!c.advance(1,1089).shiftClosed,"89");check(c.advance(1,1090).shiftClosed,"90");check(!c.advance(1,1199).longBegin,"199");check(c.advance(1,1200).longBegin,"200");check(!c.advance(1,1210).longBegin,"no dup long");});
scenario("GEST-STOP-UP-RAW",[]{RawGestureIngress s;check(s.accept(decodeS1Frame(frame(2,1,1))),"down");s.onHostStop();check(s.accept(decodeS1Frame(frame(2,1,0))),"UP after stop");check(s.orphanUps()==0,"not orphan");});
scenario("OUT-STOP-DEBTS-STRUCTURE",[]{Engine e;check(e.apply({1,Kind::Down,0,10,127}),"on");check(e.apply({2,Kind::Stop,0,0,0}),"stop");check(e.count()==2&&e.command(0).kind==CommandKind::On&&e.command(1).kind==CommandKind::Off,"debt");check(e.apply({3,Kind::Up,0,10,0}),"true UP");check(e.count()==2,"duplicate OFF");});
scenario("OBS-UNKNOWN",[]{EmptyObservationProfile p;check(p.config()==-2,"profile not unknown");check(!p.receive(14,74,64),"unconfigured RX admitted");});
scenario("OBS-UNIQUE-BINDING",[]{EmptyObservationProfile p;check(!p.receive(14,74,64)&&p.rejected()==1,"blank backend must reject");});
scenario("OBS-TX-NOT-RX-STRUCTURE",[]{Engine e;check(e.apply({1,Kind::Down,0,60,127}),"emit");check(!e.state().observed[0].known,"TX as RX");check(e.apply({2,Kind::Feedback,0,0,70}),"feedback");check(e.state().observed[0].known&&e.state().observed[0].value==70,"real RX");});
scenario("LED-CHANNEL",[]{auto r=ledBinding(0,1),g=ledBinding(0,3);check(r.offStatus==142&&r.onStatus==158&&r.note==36,"red 0");check(g.note==108,"green 0");check(ledBinding(32,2).note==-1,"32 amber");check(ledBinding(33,2).note==19,"special amber");});
scenario("LED-ATOMIC",[]{LedImage led;auto a=image();a[8]=1;check(led.accept(a)&&led.color(0)==1,"init valid");a[10]=3;a[9]=2;check(!led.accept(a)&&led.color(0)==1&&led.color(1)==0,"partial LED commit");a=image();a[8]=4;a[9]=1;check(!led.accept(a),"unknown animated");a=image();a[8+2*32]=2;check(!led.accept(a),"invalid pos32 amber");});
scenario("LED-STOP-FLASH",[]{LedImage l;auto a=image();a[10]=1;check(l.accept(a),"initial");l.flash(1,99999);l.onHostStop();check(l.flashUntil(1)==0&&l.color(1)==1,"STOP flash persistent");});
scenario("E11-MEM-INDEX-RANGE",[]{const auto a=memoryIndex(0,3),b=memoryIndex(12,2),c=memoryIndex(12,3);check(a.valid&&a.record==87&&a.count==9,"owner0 slot3");check(b.valid&&b.record==134&&b.base==450&&b.count==9,"owner12 slot2");check(!c.valid,"owner12 slot3");});
scenario("E11-MEM-PAGE-IDENTITY",[]{ // source S3 @w7 index t3..6, positions t3/t4 skip 4 and 8
 check(memoryPageIndex(3,1)==0&&memoryPageIndex(3,5)==3&&memoryPageIndex(4,1)==6&&memoryPageIndex(5,1)==12&&memoryPageIndex(6,8)==27,"page index");check(memoryPageIndex(3,4)==-1,"excluded physical position");});
scenario("S8-WIDTHS",[]{auto d0=s8BankLayout(0,0),d8=s8BankLayout(8,2),d9=s8BankLayout(9,127),d10=s8BankLayout(10,7);check(d0.valid&&d0.width==60&&d0.fieldWidth==30&&d0.modeWidth==24,"d0");check(d8.valid&&d8.width==28&&d8.modeWidth==12&&d8.start==2,"d8");check(d9.valid&&d9.width==30&&d9.groups==8&&d9.count==240,"d9 all");check(d10.valid&&d10.width==30&&d10.start==7,"d10");check(!s8BankLayout(8,8).valid&&!s8BankLayout(7,1).valid,"bad sub");});
scenario("S8-BANK-UNKNOWN",[]{auto l=s8BankLayout(8,0);std::array<int,240> a{};check(validateS8Image(l,a)==BankScan::Complete,"complete");a[0]=-2;check(validateS8Image(l,a)==BankScan::Unknown,"unknown");a[0]=-1;check(validateS8Image(l,a)==BankScan::Corrupt,"invalid -1");});
scenario("S8-BANK-BINARY",[]{auto l=s8BankLayout(8,0);std::array<int,240> a{};a[16]=2;check(validateS8Image(l,a)==BankScan::Corrupt,"binary high");a[16]=1;check(validateS8Image(l,a)==BankScan::Complete,"binary valid");auto d0=s8BankLayout(0,0);a.fill(0);a[18]=2;check(validateS8Image(d0,a)==BankScan::Corrupt,"d0 first subblock binary");a[18]=1;a[48]=2;check(validateS8Image(d0,a)==BankScan::Corrupt,"d0 second subblock binary");a[48]=1;check(validateS8Image(d0,a)==BankScan::Complete,"d0 valid binary twice");});
scenario("TR-BAR-ORDER-STRUCTURE",[]{Engine e;check(e.apply({1,Kind::Defer,1,74,20}),"defer1");check(e.apply({2,Kind::Defer,0,71,30}),"defer2");check(e.apply({3,Kind::Bar,0,0,0}),"BAR");check(e.count()==2&&e.command(0).destination==1&&e.command(1).destination==0,"BAR order");});
scenario("TR-STOP-BAR-STRUCTURE",[]{Engine e;check(e.apply({1,Kind::Defer,1,74,20}),"defer");check(e.apply({2,Kind::Stop,0,0,0}),"STOP");check(e.apply({3,Kind::Bar,0,0,0}),"BAR");check(e.count()==0,"fictional commit");});
std::cout<<"PASS "<<verified<<" native C++ scenario assertions (partial functional slices + Engine structure)\n";
return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
