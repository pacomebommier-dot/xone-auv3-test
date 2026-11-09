#include "../Core/CoreCoordinator.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace xone::l01;
static int tests=0;
static void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
template<class F>static void test(const char*name,F fn){fn();++tests;std::cout<<"PASS "<<name<<"\n";}
static std::array<int,20> frame(int addr,int value,int epoch=1,int config=7,bool known=true,bool configured=true){
 std::array<int,20> r{};const int header[9]={125,88,75,50,7,13,4,6,20};
 for(int i=0;i<9;++i){r[i]=header[i];}
 r[9]=addr%128;r[10]=addr/128;
 r[11]=known?1:0;r[12]=known?value:0;r[13]=127;r[14]=configured?1:0;r[15]=configured?config:0;
 for(int j=0;j<4;++j){r[16+j]=epoch%128;epoch/=128;}return r;
}
static CoreCoordinator ready(){CoreCoordinator c;c.enableDiagnosticLive(true);check(c.configureFxOriginForDiagnostic(42),"origin");return c;}
static FxS6::MemorySnapshot filled(int config=7,int value=9){FxS6::MemorySnapshot m{};m.supplied=true;m.valid=true;m.config=config;m.revision=1;m.values.fill(value);for(int i=6;i<9;++i)m.values[static_cast<std::size_t>(i)]=1;return m;}
static std::array<LiveBinding,36> emptyBind(){return {};}
int main(){try{
test("initial-28x9-unknown",[](){FxS6 f;for(int o=0;o<28;++o)for(int k=0;k<9;++k)check(f.field(o,k)==-2,"initial");});
test("owner-slot-index-96-unique",[](){bool seen[96]={};for(int o=0;o<28;++o)for(int s=0;s<(o<12?4:3);++s){int idx=FxS6::slotIndex(o,s);check(idx>=0&&idx<96&&!seen[idx],"slot index");seen[idx]=true;}for(bool v:seen)check(v,"gap");});
test("slot-border-rejections",[](){check(FxS6::slotIndex(12,3)==-1&&FxS6::slotIndex(11,3)==47&&FxS6::slotIndex(27,2)==95&&FxS6::slotIndex(-1,0)==-1,"slots");});
test("owner-field-252-bijection",[](){bool seen[2048]={};for(int o=0;o<28;++o)for(int k=0;k<9;++k){int a=FxS6::addressFor(o,k);int outO=-1,outK=-1;check(a>=0&&a<2048&&!seen[a],"address");seen[a]=true;check(FxS6::decodeAddress(a,outO,outK)&&outO==o&&outK==k,"inverse");}});
test("literal-source-address-samples",[](){check(FxS6::addressFor(0,0)==1552&&FxS6::addressFor(0,6)==1768&&FxS6::addressFor(3,0)==1576&&FxS6::addressFor(6,0)==1600&&FxS6::addressFor(12,0)==1648&&FxS6::addressFor(27,8)==1863,"literal");});
test("source-gaps-rejected",[](){int o,k;check(!FxS6::decodeAddress(1570,o,k)&&!FxS6::decodeAddress(1768+3*3,o,k)&&!FxS6::decodeAddress(1551,o,k)&&!FxS6::decodeAddress(1744,o,k),"gap");});
test("no-production-fx-events",[](){CoreCoordinator c;FxEvent e{0,0,9,7,true,1};check(!c.onFxEventForDiagnostic(42,e)&&c.nativeFx().epoch()==0,"production");});
test("configured-origin-required",[](){CoreCoordinator c;c.enableDiagnosticLive(true);FxEvent e{0,0,9,7,true,1};check(!c.onFxEventForDiagnostic(42,e)&&!c.configureFxOriginForDiagnostic(0),"origin");});
test("wrong-origin-rejected",[](){auto c=ready();FxEvent e{0,0,9,7,true,1};check(!c.onFxEventForDiagnostic(41,e)&&c.nativeFx().epoch()==0,"spoof");});
test("legacy-adapter-frame-decoded",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,9))&&c.nativeFx().field(0,0)==9,"frame");});
test("legacy-frame-wrong-signature",[](){auto c=ready();auto f=frame(1552,9);f[5]=12;check(!c.onFxFrameForDiagnostic(42,f),"signature");});
test("legacy-frame-wrong-length-marker",[](){auto c=ready();auto f=frame(1552,9);f[8]=19;check(!c.onFxFrameForDiagnostic(42,f),"length");});
test("legacy-frame-non-midi-byte",[](){auto c=ready();auto f=frame(1552,9);f[19]=128;check(!c.onFxFrameForDiagnostic(42,f),"byte");});
test("legacy-frame-flag-values",[](){auto c=ready();auto f=frame(1552,9);f[11]=2;check(!c.onFxFrameForDiagnostic(42,f),"known flag");});
test("legacy-frame-unknown-value-must-zero",[](){auto c=ready();auto f=frame(1552,9);f[11]=0;check(!c.onFxFrameForDiagnostic(42,f),"unknown padding");});
test("legacy-frame-unknown-config-must-zero",[](){auto c=ready();auto f=frame(1552,9);f[14]=0;check(!c.onFxFrameForDiagnostic(42,f),"config padding");});
test("legacy-frame-key-127-required",[](){auto c=ready();auto f=frame(1552,9);f[13]=1;check(!c.onFxFrameForDiagnostic(42,f),"key");});
test("legacy-frame-zero-epoch",[](){auto c=ready();check(!c.onFxFrameForDiagnostic(42,frame(1552,9,0)),"zero");});
test("legacy-frame-upper-epoch-bound",[](){auto c=ready();check(!c.onFxFrameForDiagnostic(42,frame(1552,9,16777214)),"upper epoch");});
test("legacy-frame-disallowed-gap",[](){auto c=ready();check(!c.onFxFrameForDiagnostic(42,frame(1570,9)),"gap");});
test("legacy-frame-non-fx-address",[](){auto c=ready();check(!c.onFxFrameForDiagnostic(42,frame(500,9)),"range");});
test("fx-binary-field-valid",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1768,1)),"binary");});
test("fx-binary-field-over-1",[](){auto c=ready();check(!c.onFxFrameForDiagnostic(42,frame(1768,2)),"invalid binary");});
test("fx-regular-field-127",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,127)),"regular");});
test("FX-unknown-flag-overrides-value",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,99,1,7,false))&&c.nativeFx().field(0,0)==-2,"unknown");});
test("FX-unknown-config-holds-no-match",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,9,1,0,true,false))&&c.nativeFx().config()==0&&c.nativeFx().status(0).equal==-2,"config unknown");});
test("FX-epoch-monotonic",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,10,2))&&!c.onFxFrameForDiagnostic(42,frame(1552,11,1))&&c.nativeFx().field(0,0)==10,"stale");});
test("FX-same-epoch-permitted",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,10,2))&&c.onFxFrameForDiagnostic(42,frame(1553,11,2)),"same epoch");});
test("FX-config-change-all-96-statuses",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,10,1,7))&&c.nativeFx().pending()==96,"all");});
test("FX-same-config-owner-only",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,10,1,7)),"init");c.drainFxStatusesForDiagnostic();check(c.onFxFrameForDiagnostic(42,frame(1553,11,2,7))&&c.nativeFx().pending()==4,"owner");});
test("FX-upper-owner-3-statuses",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1648,10,1,7)),"init");c.drainFxStatusesForDiagnostic();check(c.onFxFrameForDiagnostic(42,frame(1649,11,2,7))&&c.nativeFx().pending()==3,"slot");});
test("FX-profile-change-refresh-96",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,10,1,7)),"init");c.drainFxStatusesForDiagnostic();check(c.onFxFrameForDiagnostic(42,frame(1553,12,2,8))&&c.nativeFx().pending()==96,"profile");});
test("FX-status-bounded-transactional",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,10,1)),"init");for(int i=2;i<=73;++i){check(c.onFxFrameForDiagnostic(42,frame(1552,10,i)),"fill");}auto size=c.nativeFx().pending();auto epoch=c.nativeFx().epoch();check(!c.onFxFrameForDiagnostic(42,frame(1553,11,74))&&c.nativeFx().pending()==size&&c.nativeFx().epoch()==epoch,"atomic cap");});
test("FX-drain-resets-queue-not-live",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,10)),"first");c.drainFxStatusesForDiagnostic();check(c.nativeFx().pending()==0&&c.nativeFx().field(0,0)==10,"drain");});
test("FX-status-snapshot-explicit",[](){auto c=ready();auto hdr=std::array<int,8>{125,88,75,50,7,17,7,6};check(c.refreshFxStatusesForDiagnostic(42,hdr)&&c.nativeFx().pending()==96,"refresh");});
test("FX-status-snapshot-wrong-origin",[](){auto c=ready();auto hdr=std::array<int,8>{125,88,75,50,7,17,7,6};check(!c.refreshFxStatusesForDiagnostic(40,hdr)&&c.nativeFx().pending()==0,"origin");});
test("FX-status-snapshot-wrong-marker",[](){auto c=ready();auto hdr=std::array<int,8>{125,88,75,50,7,17,7,6};hdr[5]=18;check(!c.refreshFxStatusesForDiagnostic(42,hdr),"marker");});
test("memory-unknown-until-declared",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,9)),"emit");auto st=c.nativeFx().status(0);check(st.valid==-2&&st.equal==-2&&st.epoch==1,"memory default");});
test("memory-declared-invalid-is-zero",[](){auto c=ready();FxS6::MemorySnapshot m{};m.supplied=true;check(c.supplyFxMemoryForDiagnostic(0,0,m),"empty memory");check(c.onFxFrameForDiagnostic(42,frame(1552,9)),"emit");check(c.nativeFx().status(0).valid==0&&c.nativeFx().status(0).equal==-2,"empty not valid");});
test("memory-valid-profile-mismatch-unknown",[](){auto c=ready();check(c.supplyFxMemoryForDiagnostic(0,0,filled(8)),"mem");check(c.onFxFrameForDiagnostic(42,frame(1552,9,1,7)),"fx");auto s=c.nativeFx().status(0);check(s.valid==-2&&s.equal==-2,"config mismatch");});
test("memory-invalid-binary-rejected",[](){auto c=ready();auto m=filled();m.values[7]=3;check(!c.supplyFxMemoryForDiagnostic(0,0,m),"typed binary");});
test("memory-invalid-config-rejected",[](){auto c=ready();auto m=filled();m.config=0;check(!c.supplyFxMemoryForDiagnostic(0,0,m),"meta");});
test("memory-null-revision-rejected",[](){auto c=ready();auto m=filled();m.revision=0;check(!c.supplyFxMemoryForDiagnostic(0,0,m),"rev");});
test("memory-slot3-for-upper-owner-invalid",[](){auto c=ready();check(!c.supplyFxMemoryForDiagnostic(12,3,filled()),"slot 3");});
test("memory-known-mismatch-after-all-9",[](){auto c=ready();check(c.supplyFxMemoryForDiagnostic(0,0,filled()),"mem");for(int field=0;field<9;++field)check(c.onFxFrameForDiagnostic(42,frame(FxS6::addressFor(0,field),field==0?10:(field>=6?1:9),field+1)),"field");auto st=c.nativeFx().status(c.nativeFx().pending()-4);check(st.valid==1&&st.equal==0,"mismatch");});
test("memory-unknown-overrides-mismatch",[](){auto c=ready();check(c.supplyFxMemoryForDiagnostic(0,0,filled()),"mem");for(int field=0;field<8;++field)check(c.onFxFrameForDiagnostic(42,frame(FxS6::addressFor(0,field),field==0?10:(field>=6?1:9),field+1)),"field");auto st=c.nativeFx().status(c.nativeFx().pending()-4);check(st.valid==1&&st.equal==-2,"unknown");});
test("memory-all-9-match",[](){auto c=ready();check(c.supplyFxMemoryForDiagnostic(0,0,filled()),"mem");for(int field=0;field<9;++field)check(c.onFxFrameForDiagnostic(42,frame(FxS6::addressFor(0,field),field>=6?1:9,field+1)),"field");auto st=c.nativeFx().status(c.nativeFx().pending()-4);check(st.valid==1&&st.equal==1,"exact match");});
test("LIVE-unconfigured-no-fake-observation",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,9)),"frame");check(!c.nativeLive().observed(1552).known&&c.nativeLive().match(1552)==-2,"no forged observation");});
test("LIVE-port-authenticated-observation",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind()),"backend");check(c.onFxFrameForDiagnostic(42,frame(1552,9,1,7))&&c.nativeLive().observed(1552).value==9,"live");});
test("LIVE-wrong-profile-no-observation",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(8,42,emptyBind()),"backend");check(c.onFxFrameForDiagnostic(42,frame(1552,9,1,7))&&!c.nativeLive().observed(1552).known,"profile");});
test("LIVE-wrong-fx-port-vs-backend",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,9,emptyBind()),"backend");check(c.onFxFrameForDiagnostic(42,frame(1552,9,1,7))&&!c.nativeLive().observed(1552).known,"port");});
test("LIVE-FX-known-UNKNOWN-is-not-zero",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind()),"backend");check(c.onFxFrameForDiagnostic(42,frame(1552,9))&&c.onFxFrameForDiagnostic(42,frame(1552,9,2,7,false))&&!c.nativeLive().observed(1552).known,"unknown");});
test("LIVE-FX-match-only-with-verified-desired",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind())&&c.commitNativeLive(1552,9,0,7),"setup");check(c.nativeLive().match(1552)==-2,"not confirmed");check(c.onFxFrameForDiagnostic(42,frame(1552,9))&&c.nativeLive().match(1552)==1,"verified match");});
test("LIVE-FX-different-verified-values",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind())&&c.commitNativeLive(1552,8,0,7),"setup");check(c.onFxFrameForDiagnostic(42,frame(1552,9))&&c.nativeLive().match(1552)==0,"verified mismatch");});
test("LIVE-FX-unknown-desired-profile",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind())&&c.commitNativeLive(1552,9,0),"setup");check(c.onFxFrameForDiagnostic(42,frame(1552,9))&&c.nativeLive().match(1552)==-2,"desired provenance");});
test("LIVE-FX-revoke-on-config-change",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind()),"backend");check(c.onFxFrameForDiagnostic(42,frame(1552,9,1,7))&&c.nativeLive().observed(1552).known,"first");c.drainFxStatusesForDiagnostic();check(c.onFxFrameForDiagnostic(42,frame(1553,8,2,8))&&!c.nativeLive().observed(1552).known&&!c.nativeLive().observed(1553).known,"revoke");});
test("LIVE-FX-initial-reconfiguration-blocks-stale",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind()),"backend");check(c.onFxFrameForDiagnostic(42,frame(1552,9,2,7)),"fx");check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind())&&!c.nativeLive().observed(1552).known,"reset");});
test("LIVE-FX-does-not-change-desired-revision",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind())&&c.commitNativeLive(1552,9,0,7),"setup");check(c.onFxFrameForDiagnostic(42,frame(1552,9))&&c.nativeLive().revision()==1,"single writer");});
test("FX-no-musical-MIDI-emission",[](){auto c=ready();c.enableTestMidiProjection(true);check(c.onFxFrameForDiagnostic(42,frame(1552,9))&&c.output().emitted()==0,"inert");});
test("FX-reconfigure-never-forges-memory",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,9,1,7))&&c.nativeFx().status(0).valid==-2,"no memory");});
test("FX-STOP-retains-desired-data",[](){auto c=ready();check(c.commitNativeLive(1552,9,0,7),"commit");check(c.onHostStop()&&c.nativeLive().desired(1552).value==9,"stop");});
test("FX-adapter-reconnect-resets-epoch",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,9,50,7)),"old");check(c.configureFxOriginForDiagnostic(43)&&c.configureFxOriginForDiagnostic(42),"reorigin");check(c.onFxFrameForDiagnostic(42,frame(1552,10,1,7))&&c.nativeFx().epoch()==1,"new epoch");});
test("FX-backend-reconfigure-resets-epoch",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind()),"cfg");check(c.onFxFrameForDiagnostic(42,frame(1552,9,50,7)),"old");check(c.configureBackendObservationsForDiagnostic(8,42,emptyBind()),"new cfg");check(c.onFxFrameForDiagnostic(42,frame(1552,10,1,8))&&c.nativeFx().epoch()==1,"new stream");});
test("FX-backend-invalid-reconfigure-does-not-reset",[](){auto c=ready();check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind()),"cfg");check(c.onFxFrameForDiagnostic(42,frame(1552,9,50,7)),"old");check(!c.configureBackendObservationsForDiagnostic(0,42,emptyBind())&&c.nativeFx().epoch()==50,"no reset on invalid cfg");});
test("FX-status-snapshot-limit-atomic",[](){auto c=ready();auto hdr=std::array<int,8>{125,88,75,50,7,17,7,6};for(int i=0;i<4;++i)check(c.refreshFxStatusesForDiagnostic(42,hdr),"refresh");check(c.nativeFx().pending()==384&&!c.refreshFxStatusesForDiagnostic(42,hdr)&&c.nativeFx().pending()==384,"bounded snapshot");});
test("FX-manual-input-invalid-owner-atomic",[](){auto c=ready();FxEvent e{28,0,9,7,true,1};check(!c.onFxEventForDiagnostic(42,e)&&c.nativeFx().epoch()==0,"invalid owner");});
test("FX-manual-input-invalid-binary-atomic",[](){auto c=ready();FxEvent e{0,8,4,7,true,1};check(!c.onFxEventForDiagnostic(42,e)&&c.nativeFx().pending()==0,"invalid binary");});
test("FX-manual-unknown-must-minus2",[](){auto c=ready();FxEvent e{0,0,0,7,false,1};check(!c.onFxEventForDiagnostic(42,e),"unknown sentinel");});
test("FX-bridge-no-output-with-duplicate-epoch",[](){auto c=ready();c.enableTestMidiProjection(true);check(c.configureBackendObservationsForDiagnostic(7,42,emptyBind()),"cfg");check(c.onFxFrameForDiagnostic(42,frame(1552,9,1,7))&&c.onFxFrameForDiagnostic(42,frame(1553,10,1,7))&&c.output().emitted()==0,"no midi");});
test("FX-config-change-invalidates-other-FX-fields",[](){auto c=ready();for(int field=0;field<9;++field)check(c.onFxFrameForDiagnostic(42,frame(FxS6::addressFor(0,field),field>=6?1:9,field+1,7)),"old values");c.drainFxStatusesForDiagnostic();check(c.supplyFxMemoryForDiagnostic(0,0,filled(8)),"new bank");check(c.onFxFrameForDiagnostic(42,frame(1552,9,10,8)),"new config");check(c.nativeFx().field(0,1)==-2,"old field retained");check(c.nativeFx().status(0).valid==1&&c.nativeFx().status(0).equal==-2,"false match");});
test("FX-config-missing-invalidates-previous-fields",[](){auto c=ready();check(c.onFxFrameForDiagnostic(42,frame(1552,9,1,7)),"old");c.drainFxStatusesForDiagnostic();check(c.onFxFrameForDiagnostic(42,frame(1553,9,2,0,true,false))&&c.nativeFx().field(0,0)==-2,"unknown config");});
std::cout<<"PASS "<<tests<<" L01/10 FX S6 / LIVE provenance and status scenarios\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
