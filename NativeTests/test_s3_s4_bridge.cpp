#include "../Core/CoreCoordinator.hpp"
#include <array>
#include <stdexcept>
#include <iostream>
using namespace xone::l01;
static int count;
static void ok(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>static void check(const char*label,F f){f();++count;std::cout<<"PASS "<<label<<'\n';}
static std::array<LogicalIntent,8> one(){std::array<LogicalIntent,8> a{};a[0]={10,4,64,127};return a;}
static CoreCoordinator ready(){CoreCoordinator c;c.enableDiagnosticQualifiedNav(true);c.enableDiagnosticLive(true);ok(c.initializeSourceDefaultNav(),"nav");ok(c.configureBackendObservationsForDiagnostic(7,42,{}),"cfg");c.enableTestMidiProjection(true);return c;}
int main(){try{
check("batch explicit valid CC produces test trace",[](){auto c=ready();ok(c.projectQualifiedS3BatchForDiagnostic(one(),1,7,0),"emit");ok(c.output().emitted()==1&&c.output().packet(0).status==182&&c.output().packet(0).number==4&&c.output().packet(0).value==64,"packet");});
check("batch rejects stale LIVE revision",[](){auto c=ready();ok(c.commitNativeLive(70,12,0,7),"rev");ok(!c.projectQualifiedS3BatchForDiagnostic(one(),1,7,0)&&c.output().emitted()==0,"stale");});
check("batch rejects incorrect profile",[](){auto c=ready();ok(!c.projectQualifiedS3BatchForDiagnostic(one(),1,8,0)&&c.output().emitted()==0,"profile");});
check("batch rejects without NAV",[](){CoreCoordinator c;c.enableDiagnosticQualifiedNav(true);c.enableDiagnosticLive(true);ok(c.configureBackendObservationsForDiagnostic(7,42,{}),"cfg");c.enableTestMidiProjection(true);ok(!c.projectQualifiedS3BatchForDiagnostic(one(),1,7,0),"nav guard");});
check("batch rejects disabled output",[](){auto c=ready();c.enableTestMidiProjection(false);ok(!c.projectQualifiedS3BatchForDiagnostic(one(),1,7,0)&&c.output().emitted()==0,"disabled");});
check("batch rejects without qualified diagnostic",[](){auto c=ready();c.enableDiagnosticQualifiedNav(false);ok(!c.projectQualifiedS3BatchForDiagnostic(one(),1,7,0),"disabled");});
check("batch rejects invalid MIDI with complete rollback",[](){auto c=ready();auto a=one();a[1]={10,569,12,127};ok(!c.projectQualifiedS3BatchForDiagnostic(a,2,7,0),"reject");ok(c.output().emitted()==0,"no MIDI");ok(c.projectQualifiedS3BatchForDiagnostic(one(),1,7,0)&&c.output().emitted()==1,"no poisoned dedup");});
check("batch valid pair preserves order",[](){auto c=ready();auto a=one();a[1]={10,8,56,127};ok(c.projectQualifiedS3BatchForDiagnostic(a,2,7,0)&&c.output().emitted()==2,"count");ok(c.output().packet(0).number==4&&c.output().packet(1).number==8,"order");});
check("batch forbids zero and overflow",[](){auto c=ready();ok(!c.projectQualifiedS3BatchForDiagnostic(one(),0,7,0)&&!c.projectQualifiedS3BatchForDiagnostic(one(),9,7,0),"bounds");});
check("physical gesture never auto routes S4",[](){auto c=ready();ok(c.onMidi({14,0x90,13,127},0),"physical");ok(c.output().emitted()==0,"ghost output");});
check("batch refuses during LONG",[](){auto c=ready();ok(c.onMidi({14,144,13,127},0)&&c.onTimer(200)&&c.longNavPending(),"LONG");ok(!c.projectQualifiedS3BatchForDiagnostic(one(),1,7,0)&&c.output().emitted()==0,"guard");});
check("STOP keeps no fake output",[](){auto c=ready();ok(c.onHostStop()&&c.output().emitted()==0,"stop");});
std::cout<<"PASS "<<count<<" L01/11 staged S3/S4 bridge diagnostic scenarios\n";return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
