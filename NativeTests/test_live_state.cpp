#include "../Core/CoreCoordinator.hpp"
#include <iostream>
#include <stdexcept>
using namespace xone::l01;
static int tests=0;
static void test(const char *name,void (*fun)()){fun();++tests;std::cout<<"PASS "<<name<<"\n";}
static void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
static std::array<LiveBinding,36> bind(int a=28){std::array<LiveBinding,36>b{};b[0]={0,74,a,true};return b;}
static CoreCoordinator ready(){CoreCoordinator c;c.enableDiagnosticLive(true);return c;}
int main(){try{
test("UNKNOWN-at-initialization",[](){CoreCoordinator c;check(c.nativeLive().desired(28).value==-2&&c.nativeLive().observed(28).value==-2&&c.nativeLive().match(28)==-2,"unknown");});
test("no-production-LIVE-writes",[](){CoreCoordinator c;check(!c.commitNativeLive(28,64,0)&&c.nativeLive().revision()==0,"production write");});
test("first-writer-commit",[](){auto c=ready();check(c.commitNativeLive(28,64,0)&&c.nativeLive().desired(28).value==64&&c.nativeLive().revision()==1,"write");});
test("no-double-writer-stale-revision",[](){auto c=ready();check(c.commitNativeLive(28,64,0)&&!c.commitNativeLive(28,70,0)&&c.nativeLive().desired(28).value==64,"stale");});
test("command-invalid-no-mutation",[](){auto c=ready();check(!c.commitNativeLive(2048,10,0)&&!c.commitNativeLive(28,-2,0)&&!c.commitNativeLive(28,128,0)&&c.nativeLive().revision()==0,"invalid write");});
test("desired-never-hardware-observed",[](){auto c=ready();check(c.commitNativeLive(28,64,0)&&c.nativeLive().match(28)==-2,"false feedback");});
test("missing-backend-profile-refuses-feedback",[](){auto c=ready();check(!c.onBackendMidiForDiagnostic(7,0,176,74,64,1),"unconfigured feedback");});
test("configured-feedback-separate-from-desired",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind())&&c.onBackendMidiForDiagnostic(7,0,176,74,64,1),"observe");check(c.nativeLive().observed(28).value==64&&!c.nativeLive().desired(28).known&&c.nativeLive().match(28)==-2,"separation");});
test("backend-port-auth-not-channel",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind())&&!c.onBackendMidiForDiagnostic(8,0,176,74,64,1),"port spoof");});
test("wrong-channel-command-CC-refused",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind()),"profile");check(!c.onBackendMidiForDiagnostic(7,1,176,74,64,1)&&!c.onBackendMidiForDiagnostic(7,0,144,74,64,1)&&!c.onBackendMidiForDiagnostic(7,0,176,75,64,1),"wrong MIDI");});
test("unknown-input-address-not-matched",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind()),"profile");check(!c.onBackendMidiForDiagnostic(7,0,176,71,64,1)&&!c.nativeLive().observed(71).known,"invented mapping");});
test("backend-stale-epoch-rejected",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind()),"profile");check(c.onBackendMidiForDiagnostic(7,0,176,74,70,3)&&!c.onBackendMidiForDiagnostic(7,0,176,74,71,2)&&c.nativeLive().observed(28).value==70,"epoch");});
test("backend-duplicate-epoch-accepted",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind()),"profile");check(c.onBackendMidiForDiagnostic(7,0,176,74,70,3)&&c.onBackendMidiForDiagnostic(7,0,176,74,71,3)&&c.nativeLive().observed(28).value==71,"duplicate epoch allowed");});
test("known-match-only-with-both-sides",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind())&&c.commitNativeLive(28,64,0,2)&&c.onBackendMidiForDiagnostic(7,0,176,74,64,1)&&c.nativeLive().match(28)==1,"match");});
test("matching-value-with-unknown-profile-is-UNKNOWN",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind())&&c.commitNativeLive(28,64,0)&&c.onBackendMidiForDiagnostic(7,0,176,74,64,1)&&c.nativeLive().match(28)==-2,"unverified desired config");});
test("matching-value-with-wrong-profile-is-UNKNOWN",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind())&&c.commitNativeLive(28,64,0,3)&&c.onBackendMidiForDiagnostic(7,0,176,74,64,1)&&c.nativeLive().match(28)==-2,"wrong profile");});
test("known-mismatch",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind())&&c.commitNativeLive(28,63,0,2)&&c.onBackendMidiForDiagnostic(7,0,176,74,64,1)&&c.nativeLive().match(28)==0,"diff");});
test("reconfigure-invalidates-observed",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind())&&c.onBackendMidiForDiagnostic(7,0,176,74,64,1),"observe");check(c.configureBackendObservationsForDiagnostic(3,7,bind())&&!c.nativeLive().observed(28).known,"reconfigure");});
test("disconnect-invalidates-observed",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(2,7,bind())&&c.onBackendMidiForDiagnostic(7,0,176,74,64,1),"observe");c.backendDisconnectedForDiagnostic();check(!c.nativeLive().observed(28).known&&c.nativeLive().backendConfig()==-2,"disconnect");});
test("profile-duplicate-input-rejected",[](){auto c=ready();auto b=bind();b[1]={0,74,29,true};check(!c.configureBackendObservationsForDiagnostic(2,7,b),"duplicate source");});
test("profile-duplicate-target-rejected",[](){auto c=ready();auto b=bind();b[1]={0,75,28,true};check(!c.configureBackendObservationsForDiagnostic(2,7,b),"duplicate target");});
test("profile-invalid-port-config-rejected",[](){auto c=ready();check(!c.configureBackendObservationsForDiagnostic(0,7,bind())&&!c.configureBackendObservationsForDiagnostic(2,0,bind()),"invalid profile");});
test("no-reset-LIVE-on-stop",[](){auto c=ready();check(c.commitNativeLive(28,64,0)&&c.onHostStop()&&c.nativeLive().desired(28).value==64,"STOP");});
test("session-invalid-schema-atomic",[](){auto c=ready();check(c.commitNativeLive(28,64,0),"first");auto img=c.nativeLive().snapshot();img.schema=2;check(!c.restoreNativeSessionForDiagnostic(img)&&c.nativeLive().desired(28).value==64,"bad schema");});
test("session-invalid-data-atomic",[](){auto c=ready();check(c.commitNativeLive(28,64,0),"first");auto img=c.nativeLive().snapshot();img.schema=1;img.config=7;img.revision=1;img.desired[28]=128;check(!c.restoreNativeSessionForDiagnostic(img)&&c.nativeLive().desired(28).value==64,"bad data");});
test("session-restore-preserves-unpatched-LIVE",[](){auto c=ready();LiveState::SessionImage i{};i.desired.fill(-2);i.schema=1;i.config=7;i.revision=5;i.desired[28]=91;check(c.restoreNativeSessionForDiagnostic(i)&&c.nativeLive().desired(28).value==91&&c.nativeLive().sessionConfig()==7&&c.nativeLive().match(28)==-2,"restore");});
test("session-restore-does-not-send-MIDI",[](){auto c=ready();LiveState::SessionImage i{};i.desired.fill(-2);i.schema=1;i.config=7;i.revision=1;i.desired[28]=91;c.enableTestMidiProjection(true);check(c.restoreNativeSessionForDiagnostic(i)&&c.output().emitted()==0,"auto send");});
test("spot-source-formula-inst",[](){int a=-1,b=-1;check(LiveState::spotAddresses(3*1024+32+1,0,a,b)&&a==16&&b==28,"@zs INST");});
test("spot-source-formula-sample",[](){int a=-1,b=-1;check(LiveState::spotAddresses(3*1024+64+8,11,a,b)&&a==643&&b==655,"@zs SAMPLE");});
test("spot-source-formula-yellows",[](){int a=-1,b=-1;check(LiveState::spotAddresses(3*1024+96+8,2,a,b)&&a==1791&&b==-1,"@zs YELLOW");});
test("spot-source-formula-machine",[](){int a=-1,b=-1;check(LiveState::spotAddresses(24*1024+14*32,5,a,b)&&a==1869&&b==-1,"@zs MACHINE");});
test("spot-source-no-fake-invalid",[](){int a=-1,b=-1;check(!LiveState::spotAddresses(3*1024+32,0,a,b)&&a==-1&&b==-1,"bad selection");});
test("browser-no-unknown-preset",[](){auto c=ready();std::array<BrowserSpotInput,12>s{};s[0]={true,3*1024+33,0,3*1024+33,0};auto p=c.planNativeBrowserSpots(s);check(!p.valid&&p.count==0,"unknown blocks");});
test("browser-invalid-selection-blocks",[](){auto c=ready();std::array<BrowserSpotInput,12>s{};s[0]={true,3*1024+33,0,3*1024+33,1};check(!c.planNativeBrowserSpots(s).valid,"mismatch");});
test("browser-known-source-plans-not-commits",[](){auto c=ready();auto p0=3*1024+33;int state=-1,preset=-1;check(LiveState::spotAddresses(p0,0,state,preset),"route");check(c.commitNativeLive(state,1,0)&&c.commitNativeLive(preset,4,1),"cells");std::array<BrowserSpotInput,12>s{};s[0]={true,p0,0,p0,0};auto p=c.planNativeBrowserSpots(s);check(p.valid&&p.count==1&&p.changes[0].preset==1&&c.nativeLive().desired(preset).value==4,"plan");});
test("browser-atomic-commit",[](){auto c=ready();int packed=3*1024+33,a=-1,b=-1;check(LiveState::spotAddresses(packed,0,a,b),"route");check(c.commitNativeLive(a,1,0)&&c.commitNativeLive(b,4,1),"setup");std::array<BrowserSpotInput,12>s{};s[0]={true,packed,0,packed,0};auto p=c.planNativeBrowserSpots(s);check(c.commitNativeBrowserSpots(p,2)&&c.nativeLive().desired(b).value==1&&c.nativeLive().revision()==3,"commit");});
test("browser-no-partial-if-one-unknown",[](){auto c=ready();int packed=3*1024+33,a=-1,b=-1;check(LiveState::spotAddresses(packed,0,a,b),"route");check(c.commitNativeLive(a,1,0)&&c.commitNativeLive(b,4,1),"setup");std::array<BrowserSpotInput,12>s{};s[0]={true,packed,0,packed,0};s[1]={true,packed,1,packed,1};auto p=c.planNativeBrowserSpots(s);check(!p.valid&&p.count==0&&c.nativeLive().desired(b).value==4,"partial");});
test("browser-stale-revision-refused",[](){auto c=ready();int packed=3*1024+33,a=-1,b=-1;check(LiveState::spotAddresses(packed,0,a,b),"route");check(c.commitNativeLive(a,1,0)&&c.commitNativeLive(b,4,1),"setup");std::array<BrowserSpotInput,12>s{};s[0]={true,packed,0,packed,0};auto p=c.planNativeBrowserSpots(s);check(!c.commitNativeBrowserSpots(p,1)&&c.nativeLive().desired(b).value==4,"stale");});
test("browser-plan-inert-without-live",[](){CoreCoordinator c;std::array<BrowserSpotInput,12>s{};auto p=c.planNativeBrowserSpots(s);check(p.valid&&p.count==0&&c.output().emitted()==0,"inert");});
test("browser-no-musical-output",[](){auto c=ready();c.enableTestMidiProjection(true);check(c.commitNativeLive(28,65,0)&&c.output().emitted()==0,"no midi");});
std::cout<<"PASS "<<tests<<" L01/09 authenticated LIVE / BROWSER SPOT assertions\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
