#include "../Core/CoreCoordinator.hpp"
#include <iostream>
#include <stdexcept>
using namespace xone::l01;
static int cases=0;
static void req(bool b,const char*reason){if(!b)throw std::runtime_error(reason);}
static void run(const char*name,void(*f)()){f();++cases;std::cout<<"PASS "<<name<<"\n";}
static CoreCoordinator ready(bool yes=true){CoreCoordinator c;req(c.initializeSourceDefaultNav(),"default nav");c.enableDiagnosticTouchNav(yes);return c;}
static void d(CoreCoordinator&c,int n,int t){req(c.onMidi({14,144,n,127},t),"down");}
static void u(CoreCoordinator&c,int n,int t){req(c.onMidi({14,128,n,0},t),"up");}
static void selectPageHeader(CoreCoordinator&c){d(c,36,0);req(c.nativeNav().active().at(293)==1 && c.nativeNav().active().at(299)==1,"@zk primary/flag");u(c,36,20);}
int main(){try {
run("K2-TOUCH-DIAG-OFF-DEFAULT",[](){auto c=ready(false);auto old=c.nativeNav().active();d(c,36,0);u(c,36,10);req(c.nativeNav().active()==old && c.nativeTouchStats().commits==0,"unsafe default");});
run("K2-TOUCH-PAGE-SELECT-PRIMARY-P3",[](){auto c=ready();d(c,36,0);auto n=c.nativeNav().active();req(n.at(293)==1 && n.at(299)==1 && NavS3::validate(n),"page select");});
run("K2-TOUCH-PAGE-SELECT-SECONDARY-P3-V5",[](){auto c=ready();d(c,37,0);auto n=c.nativeNav().active();req(n.at(294)==5 && n.at(300)==1 && n.at(299)==0 && NavS3::validate(n),"secondary select");});
run("K2-TOUCH-CONTEXT-P5-SELECT",[](){auto c=ready();d(c,38,0);auto n=c.nativeNav().active();req(n.at(297)==1 && n.at(303)==1 && NavS3::validate(n),"context select");});
run("K2-TOUCH-PAGE-FLAG-RESELECT-DEFERRED",[](){auto c=ready();selectPageHeader(c);d(c,36,100);req(c.nativeTouchOwner()==4 && c.nativeTouchStats().deferred==1,"touch pending");});
run("K2-TOUCH-SHORT-RELEASE-CLEARS-FLAG",[](){auto c=ready();selectPageHeader(c);d(c,36,100);u(c,36,299);req(c.nativeTouchOwner()==-1 && c.nativeNav().active().at(299)==0 && c.nativeNav().active().at(293)==1,"short release");});
run("K2-TOUCH-200MS-PRESERVES-FLAG",[](){auto c=ready();selectPageHeader(c);d(c,36,100);u(c,36,300);req(c.nativeNav().active().at(299)==1 && c.nativeTouchOwner()==-1,"threshold equality");});
run("K2-TOUCH-201MS-PRESERVES-FLAG",[](){auto c=ready();selectPageHeader(c);d(c,36,100);u(c,36,301);req(c.nativeNav().active().at(299)==1,"threshold above");});
run("K2-TOUCH-SHORT-HISTORY-CHANGES-ATOMIC",[](){auto c=ready();selectPageHeader(c);auto old=c.nativeNav().active();auto revision=c.nativeNav().revision();d(c,36,100);req(c.nativeNav().active()==old && c.nativeNav().revision()==revision,"deferred changed nav");u(c,36,150);req(c.nativeNav().revision()==revision+1 && NavS3::validate(c.nativeNav().active()),"atomic release");});
run("K2-TOUCH-SAME-KEY-DOUBLE-DOWN-NO-DOUBLE",[](){auto c=ready();d(c,36,0);auto count=c.nativeTouchStats().commits;d(c,36,1);req(c.nativeTouchStats().commits==count,"duplicate committed");});
run("K2-TOUCH-WRONG-CHANNEL-INERT",[](){auto c=ready();req(!c.onMidi({13,144,36,127},0),"filter source");req(c.nativeTouchStats().commits==0 && c.nativeNav().active().at(299)==0,"wrong channel");});
run("K2-TOUCH-HELD-BOT-BLOCKS",[](){auto c=ready();d(c,13,0);d(c,36,20);req(c.nativeNav().active().at(299)==0 && c.nativeTouchStats().blocked>0,"BOT guard");});
run("K2-TOUCH-DIAGNOSTIC-GUARD-BLOCKS",[](){auto c=ready();req(c.observeTouchOwnerForDiagnostic(17),"guard");d(c,36,0);req(c.nativeNav().active().at(299)==0,"touch guard");});
run("K2-TOUCH-LONG-NAV-PENDING-BLOCKS",[](){auto c=ready();c.enableDiagnosticQualifiedNav(true);d(c,13,0);req(c.onTimer(200),"timer");d(c,36,210);req(c.nativeNav().active().at(299)==0,"long exclusion");});
run("K2-TOUCH-PENDING-BLOCKS-BOT-NAV",[](){auto c=ready();c.enableDiagnosticQualifiedNav(true);selectPageHeader(c);d(c,36,100);d(c,13,110);u(c,13,150);req(c.nativeNav().active().at(292)==0 && c.nativeTouchOwner()==4,"BOT stole touch");});
run("K2-TOUCH-STOP-DISARMS-NO-UP",[](){auto c=ready();selectPageHeader(c);d(c,36,100);req(c.onHostStop(),"STOP");req(c.nativeTouchOwner()==-1 && c.nativeNav().active().at(299)==1,"STOP erases state");u(c,36,120);req(c.nativeNav().active().at(299)==1,"phantom release");});
run("K2-TOUCH-NO-MIDI-OUTPUT",[](){auto c=ready();c.enableTestMidiProjection(true);d(c,36,0);u(c,36,20);req(c.output().emitted()==0,"invented MIDI");});
run("K2-TOUCH-RETURNS-TO-NAV-GUARD",[](){auto c=ready();c.enableDiagnosticQualifiedNav(true);selectPageHeader(c);d(c,36,100);u(c,36,150);d(c,13,170);u(c,13,200);req(c.nativeNav().active().at(292)==1,"BOT remained blocked after up");});
run("K2-TOUCH-PAGE-SELECTION-NO-LIVE-STATE",[](){auto c=ready();d(c,36,0);req(c.nativeNav().active().sampleSlot==0 && c.nativeNav().active().presetFlag==0,"invented LIVE");});
run("K2-TOUCH-INVALID-STAGE-BLOCKS",[](){auto c=ready();auto n=c.nativeNav().active();req(c.stageNativeNav(n),"stage");d(c,36,0);req(c.nativeTouchStats().commits==0,"staged conflict");});
run("K2-TOUCH-RELEASE-UNRECOGNIZED-INERT",[](){auto c=ready();u(c,36,0);req(c.nativeTouchStats().released==0 && c.nativeNav().revision()==1,"ghost release");});
run("K2-TOUCH-PAGE1-PRIMARY-GROUP4",[](){auto c=ready();auto n=c.nativeNav().active();n.at(291)=1;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"page1");d(c,36,0);req(c.nativeNav().active().at(295)==1 && c.nativeNav().active().at(301)==1,"page1 group4");});
run("K2-TOUCH-PAGE1-SECONDARY-GROUP4",[](){auto c=ready();auto n=c.nativeNav().active();n.at(291)=1;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"page1");d(c,37,0);req(c.nativeNav().active().at(296)==5 && c.nativeNav().active().at(302)==1,"page1 group4 secondary");});
run("K2-TOUCH-PAGE1-CONTEXT-GROUP6",[](){auto c=ready();auto n=c.nativeNav().active();n.at(291)=1;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"page1");d(c,38,0);req(c.nativeNav().active().at(298)==1 && c.nativeNav().active().at(304)==1,"page1 group6");});
run("K2-TOUCH-MODE2-RANK9-SELECT",[](){auto c=ready();auto n=c.nativeNav().active();n.at(292)=2;n.contextMode=2;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"mode2");d(c,36,0);req(c.nativeNav().active().at(305)==1 && c.nativeNav().active().at(309)==1,"rank9");});
run("K2-TOUCH-MODE2-RANK10-PUSHES-RANK9",[](){auto c=ready();auto n=c.nativeNav().active();n.at(292)=2;n.contextMode=2;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"mode2");d(c,36,0);u(c,36,10);d(c,37,20);req(c.nativeNav().active().at(309)==2&&c.nativeNav().active().at(310)==1,"rank priority");});
run("K2-TOUCH-MODE2-RANK10-RESELECT-REMOVES",[](){auto c=ready();auto n=c.nativeNav().active();n.at(292)=2;n.contextMode=2;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"mode2");d(c,36,0);u(c,36,10);d(c,37,20);u(c,37,30);d(c,37,40);req(c.nativeTouchOwner()==5,"rank10 deferred");u(c,37,55);req(c.nativeNav().active().at(309)==1&&c.nativeNav().active().at(310)==0,"rank removals");});
run("K2-TOUCH-MODE2-RANK10-LONG-KEEPS-RANK",[](){auto c=ready();auto n=c.nativeNav().active();n.at(292)=2;n.contextMode=2;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"mode2");d(c,37,0);u(c,37,10);d(c,37,30);u(c,37,230);req(c.nativeNav().active().at(310)==1,"200ms rank long");});
run("K2-TOUCH-MODE5-NO-SYNTHETIC-SELECT",[](){auto c=ready();auto n=c.nativeNav().active();n.at(292)=5;n.contextMode=0;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"mode5");auto before=c.nativeNav().active();d(c,36,0);req(c.nativeNav().active()==before,"k13 should not be k2");});
run("K2-TOUCH-CONCURRENT-DOWN-DOES-NOT-CLAIM-SECOND",[](){auto c=ready();selectPageHeader(c);d(c,36,100);d(c,38,110);req(c.nativeTouchOwner()==4 && c.nativeNav().active().at(303)==0,"second touch stole first");});
run("K2-TOUCH-HARDWARE-NOTE-ON-VELOCITY0-IS-UP",[](){auto c=ready();selectPageHeader(c);d(c,36,100);req(c.onMidi({14,144,36,0},150),"vel0 UP");req(c.nativeTouchOwner()==-1&&c.nativeNav().active().at(299)==0,"vel0 did not release");});
std::cout<<"PASS "<<cases<<" L01/08 source-constrained @zk/@zl TOUCH NAV assertions\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
