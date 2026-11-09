#include "../Core/CoreCoordinator.hpp"
#include <array>
#include <stdexcept>
#include <iostream>
using namespace xone::l01;
static int tests;
static void ok(bool b,const char*msg){if(!b)throw std::runtime_error(msg);}
template<class F>static void test(const char* n,F fn){fn();++tests;std::cout<<"PASS "<<n<<"\n";}
static auto mem(){return BankS6::MemoryImage{9,8,7,6,5,4,1,0,1};}
static auto bindings(){return std::array<LiveBinding,LiveState::BindingCount>{};}
static CoreCoordinator ready(){CoreCoordinator c;c.enableDiagnosticLive(true);ok(c.initializeEmptyNativeBanksForDiagnostic(),"init");ok(c.configureBackendObservationsForDiagnostic(7,42,bindings()),"profile");return c;}
int main(){try{
test("recall valid MEMORY 9 fields one LIVE revision",[](){auto c=ready();ok(c.saveNativeMemoryForDiagnostic(0,0,7,mem(),0),"mem");ok(c.recallNativeFxMemoryForDiagnostic(0,0,0),"recall");ok(c.nativeLive().revision()==1,"atomic revision");auto m=mem();for(int i=0;i<9;++i){auto a=FxS6::addressFor(0,i);ok(c.nativeLive().desired(a).known&&c.nativeLive().desired(a).value==m[i],"value");}});
test("recall no MIDI notes or CC",[](){auto c=ready();c.enableTestMidiProjection(true);ok(c.saveNativeMemoryForDiagnostic(0,0,7,mem(),0),"save");ok(c.recallNativeFxMemoryForDiagnostic(0,0,0),"recall");ok(c.output().emitted()==0,"midi");});
test("recall never manufactures observed",[](){auto c=ready();ok(c.saveNativeMemoryForDiagnostic(0,0,7,mem(),0),"save");ok(c.recallNativeFxMemoryForDiagnostic(0,0,0),"recall");ok(!c.nativeLive().observed(1552).known&&c.nativeLive().match(1552)==-2,"unknown");});
test("recall rejects invalid empty slot",[](){auto c=ready();ok(!c.recallNativeFxMemoryForDiagnostic(0,0,0),"empty");ok(c.nativeLive().revision()==0,"no mutation");});
test("recall rejects wrong profile",[](){auto c=ready();ok(c.saveNativeMemoryForDiagnostic(0,0,8,mem(),0),"save");ok(!c.recallNativeFxMemoryForDiagnostic(0,0,0),"profile");ok(!c.nativeLive().desired(1552).known,"atomic");});
test("recall rejects stale LIVE revision",[](){auto c=ready();ok(c.saveNativeMemoryForDiagnostic(0,0,7,mem(),0),"save");ok(c.recallNativeFxMemoryForDiagnostic(0,0,0),"first");ok(!c.recallNativeFxMemoryForDiagnostic(0,0,0),"stale");ok(c.nativeLive().revision()==1,"no partial");});
test("recall destination preserves unrelated cells",[](){auto c=ready();ok(c.commitNativeLive(100,77,0,7),"unrelated");ok(c.saveNativeMemoryForDiagnostic(0,0,7,mem(),0),"save");ok(c.recallNativeFxMemoryForDiagnostic(0,0,1),"recall");ok(c.nativeLive().desired(100).value==77,"unrelated");});
test("recall page upper owner 27",[](){auto c=ready();ok(c.saveNativeMemoryForDiagnostic(27,2,7,mem(),0),"save");ok(c.recallNativeFxMemoryForDiagnostic(27,2,0),"recall");ok(c.nativeLive().desired(1863).value==1,"last address");});
test("recall preserves PATCH independence",[](){auto c=ready();ok(c.saveNativeMemoryForDiagnostic(0,0,7,mem(),0),"save");ok(c.recallNativeFxMemoryForDiagnostic(0,0,0),"recall");BankS6::PatchRecord p;ok(c.nativeBank().readPatch(0,0,p)&&!p.valid,"PATCH absent");});
test("recall disabled outside diagnostic",[](){CoreCoordinator c;ok(!c.recallNativeFxMemoryForDiagnostic(0,0,0),"disabled");});
test("recall refuses while LONG pending",[](){auto c=ready();ok(c.initializeSourceDefaultNav(),"NAV");c.enableDiagnosticQualifiedNav(true);ok(c.saveNativeMemoryForDiagnostic(0,0,7,mem(),0),"save");
 ok(c.onMidi({14,144,13,127},0)&&c.onTimer(200)&&c.longNavPending(),"pending");
 ok(!c.recallNativeFxMemoryForDiagnostic(0,0,0)&&c.nativeLive().revision()==0,"refused");
 ok(c.onMidi({14,128,13,0},220)&&!c.longNavPending(),"up");
 ok(c.recallNativeFxMemoryForDiagnostic(0,0,0),"recovered");});
test("recall is not saved in a session automatically",[](){auto c=ready();ok(c.saveNativeMemoryForDiagnostic(0,0,7,mem(),0),"save");ok(c.nativeLive().snapshot().revision==0,"snapshot of LIVE not bank");});
std::cout<<"PASS "<<tests<<" L01/11 MEMORY recall scenarios\n";return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
