#include "../Core/CoreCoordinator.hpp"
#include <iostream>
#include <stdexcept>
using namespace xone::l01;
static int cases=0;
static void req(bool b,const char*m){if(!b)throw std::runtime_error(m);}
static void run(const char*name,void(*f)()){f();++cases;std::cout<<"PASS "<<name<<"\n";}
static void down(CoreCoordinator& c,int note,int t){req(c.onMidi({14,0x90,note,127},t),"DOWN");}
static void up(CoreCoordinator& c,int note,int t){req(c.onMidi({14,0x80,note,0},t),"UP");}
static CoreCoordinator ready(){CoreCoordinator c;req(c.initializeSourceDefaultNav(),"source initial nav");c.enableDiagnosticShortNav(true);return c;}
int main(){try{
run("PIPE-NAV-DEFAULT-133-VALID",[](){auto c=ready();req(c.nativeNav().stable()&&NavS3::validate(c.nativeNav().active()),"default nav");});
run("PIPE-NAV-BOT2-TO-PRESET-NO-MIDI",[](){auto c=ready();down(c,13,0);up(c,13,40);req(c.nativeNav().active().at(292)==1&&c.nativeNav().active().presetFlag==1,"mode1");req(c.output().emitted()==0,"phantom midi");});
run("PIPE-NAV-BOT2-SECOND-TO-BASE",[](){auto c=ready();down(c,13,0);up(c,13,40);down(c,13,70);up(c,13,95);req(c.nativeNav().active().at(292)==0&&c.nativeNav().active().presetFlag==0,"not toggled");});
run("PIPE-NAV-BOT3-MODE-TWO",[](){auto c=ready();down(c,14,0);up(c,14,35);req(c.nativeNav().active().at(292)==2,"mode2");});
run("PIPE-NAV-BOT4-TOGGLE-PAGE",[](){auto c=ready();down(c,15,0);up(c,15,35);req(c.nativeNav().active().at(291)==1,"page toggle");});
run("PIPE-NAV-COMBO-ROLE4-TO-BROWSER",[](){auto c=ready();down(c,13,0);down(c,14,20);up(c,14,40);up(c,13,60);req(c.nativeNav().active().at(292)==3&&c.nativeNav().active().contextMode==3,"combo4->browser");req(c.output().emitted()==0,"midi");});
run("PIPE-NAV-COMBO-ROLE6-TO-MACHINE",[](){auto c=ready();down(c,13,0);down(c,15,20);up(c,15,40);req(c.nativeNav().active().at(292)==7&&c.nativeNav().active().contextMode==7,"combo6->machine");});
run("PIPE-NAV-COMBO-ROLE9-TO-BASE",[](){auto c=ready();auto m=c.nativeNav().active();m.at(292)=3;m.contextMode=3;req(NavS3::regenerate(m)&&c.stageNativeNav(m)&&c.commitNativeNav(),"set browse");down(c,13,0);down(c,12,20);up(c,12,40);req(c.nativeNav().active().at(292)==0,"combo9->base");});
run("PIPE-NAV-LONG-DOES-NOT-SHORT-DISPATCH",[](){auto c=ready();down(c,13,0);req(c.onTimer(200),"timer");up(c,13,260);req(c.nativeNav().active().at(292)==0,"LONG treated SHORT");});
run("PIPE-NAV-STOP-HELD-INERT",[](){auto c=ready();down(c,13,0);req(c.onHostStop(),"stop");up(c,13,240);req(c.nativeNav().active().at(292)==0,"STOP created action");});
run("PIPE-NAV-DIAGNOSTIC-IS-DISABLED-BY-DEFAULT",[](){CoreCoordinator c;req(c.initializeSourceDefaultNav(),"init");down(c,13,0);up(c,13,35);req(c.nativeNav().active().at(292)==0,"unauthorised dispatch");});
run("PIPE-NAV-NO-CONFIG-NO-MUSIC",[](){CoreCoordinator c;c.enableDiagnosticShortNav(true);down(c,13,0);up(c,13,50);req(!c.nativeNav().stable()&&c.output().emitted()==0,"fake nav");});
run("PIPE-NAV-SOURCE-IMAGE-COMMIT-REFRESHES-S3",[](){auto c=ready();auto n=c.nativeNav().active();n.at(292)=7;n.contextMode=7;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"commit");req(c.s3().nav()==7&&c.nativeNav().active().at(292)==7,"s3 desync");});
run("PIPE-NAV-CORRUPT-SNAPSHOT-DID-NOT-UPDATE-S3",[](){auto c=ready();auto n=c.nativeNav().active();n.at(292)=7;n.contextMode=7;req(NavS3::regenerate(n),"regen");n.at(322)=-2;req(c.stageNativeNav(n)&&!c.commitNativeNav(),"accepted corrupted");req(c.s3().nav()==0&&c.nativeNav().active().at(292)==0,"half commit");});
run("PIPE-NAV-INVALID-CHANNEL-NO-HELD",[](){auto c=ready();req(!c.onMidi({15,0x90,13,127},0),"wrong channel");req(!c.surface().held(1)&&c.nativeNav().revision()==1,"phantom");});
run("PIPE-NAV-LOW-CAPACITY-DOES-NOT-ALTER-S3",[](){auto c=ready();for(int i=0;i<20;++i){down(c,13,2*i);up(c,13,2*i+1);}req(c.nativeNav().revision()>=2&&c.output().emitted()==0,"bad revision");});
run("PIPE-NAV-OLD-DOWN-NO-RETARGET",[](){auto c=ready();down(c,13,0);auto n=c.nativeNav().active();n.at(292)=7;n.contextMode=7;req(NavS3::regenerate(n)&&c.stageNativeNav(n)&&c.commitNativeNav(),"stage");up(c,13,40);req(c.nativeNav().active().at(292)==7,"stale down captured new nav");});
run("PIPE-NAV-STAGE-BUSY-NO-AUTO-COMMIT",[](){auto c=ready();auto n=c.nativeNav().active();n.at(313)=7;req(NavS3::regenerate(n)&&c.stageNativeNav(n),"stage");down(c,13,0);up(c,13,20);req(c.nativeNav().active().at(292)==0&&c.nativeNav().hasStage(),"pending stage stolen");req(c.commitNativeNav()&&c.nativeNav().active().at(313)==7,"commit pending lost");});
run("PIPE-NAV-COMBO-LONG-IS-INERT",[](){auto c=ready();down(c,13,0);down(c,14,20);req(c.onTimer(220),"timer");up(c,14,260);req(c.nativeNav().active().at(292)==0,"long combo treated as short");});
std::cout<<"PASS "<<cases<<" NAV integrated S3 pipeline scenarios\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
