#include "../Core/CoreCoordinator.hpp"
#include <stdexcept>
#include <iostream>
using namespace xone::l01;
static int count{};
static void verify(bool b,const char*msg){if(!b)throw std::runtime_error(msg);}
template<class F>static void check(const char*n,F f){f();++count;std::cout<<"PASS "<<n<<'\n';}
static BankS6::PatchImage patch(int o){BankS6::PatchImage a{};a.fill(-2);int n=BankS6::patchWidth(o);for(int k=0;k<n;++k){int sub=o<8?k:k%42;a[k]=o<8?(sub>=28?1:sub>=16?0:9):(sub>=30?1:sub>=18?0:9);}return a;}
static BankS6::MemoryImage memory(){return {9,8,7,6,5,4,1,0,1};}
static CoreCoordinator ready(){CoreCoordinator c;c.enableDiagnosticLive(true);verify(c.initializeEmptyNativeBanksForDiagnostic(),"initialize");return c;}
int main(){try{
check("bank starts uninitialized",[](){BankS6 b;verify(!b.initialized()&&b.generation()==0,"initial");BankS6::MemoryRecord m;verify(!b.readMemory(0,0,m),"read forbidden");});
check("explicit initialization once",[](){BankS6 b;verify(b.initializeEmpty()&&!b.initializeEmpty()&&b.generation()==1,"once");});
check("all-new memory empty invalid",[](){auto c=ready();for(int o=0;o<28;++o)for(int sl=0;sl<(o<12?4:3);++sl){BankS6::MemoryRecord m;verify(c.nativeBank().readMemory(o,sl,m)&&!m.valid&&m.revision==0,"empty mem");for(auto v:m.values)verify(v==-2,"unassigned");}});
check("all-new patch empty invalid",[](){auto c=ready();for(int o=0;o<28;++o)for(int sl=0;sl<3;++sl){BankS6::PatchRecord p;verify(c.nativeBank().readPatch(o,sl,p)&&!p.valid&&p.revision==0,"empty patch");}});
check("patch 84 indices unique",[](){bool seen[84]={};for(int o=0;o<28;++o)for(int s=0;s<3;++s){int k=BankS6::patchIndex(o,s);verify(k>=0&&k<84&&!seen[k],"index");seen[k]=true;}});
check("memory 96 indices unique",[](){bool seen[96]={};for(int o=0;o<28;++o)for(int s=0;s<(o<12?4:3);++s){int k=BankS6::memoryIndex(o,s);verify(k>=0&&k<96&&!seen[k],"index");seen[k]=true;}});
check("patch widths source",[](){verify(BankS6::patchWidth(0)==40&&BankS6::patchWidth(7)==40&&BankS6::patchWidth(8)==84&&BankS6::patchWidth(11)==84&&BankS6::patchWidth(12)==42&&BankS6::patchWidth(27)==42,"width");});
check("slot bounds",[](){verify(BankS6::memoryIndex(12,3)==-1&&BankS6::memoryIndex(11,3)==47&&BankS6::patchIndex(28,0)==-1&&BankS6::patchIndex(0,3)==-1,"bounds");});
check("valid patch each class",[](){auto c=ready();for(int o: {0,7,8,11,12,27})verify(c.saveNativePatchForDiagnostic(o,0,7,patch(o),0),"save");});
check("valid memory each class",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,3,7,memory(),0)&&c.saveNativeMemoryForDiagnostic(27,2,7,memory(),0),"save");});
check("write rejects before initialization",[](){BankS6 b;verify(!b.savePatch(0,0,7,patch(0),0)&&!b.saveMemory(0,0,7,memory(),0),"uninit");});
check("patch revision increments",[](){auto c=ready();verify(c.saveNativePatchForDiagnostic(8,2,7,patch(8),0)&&c.saveNativePatchForDiagnostic(8,2,7,patch(8),1),"save");BankS6::PatchRecord p;verify(c.nativeBank().readPatch(8,2,p)&&p.revision==2&&p.valid,"revision");});
check("memory revision increments",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0)&&c.saveNativeMemoryForDiagnostic(0,0,7,memory(),1),"save");BankS6::MemoryRecord p;verify(c.nativeBank().readMemory(0,0,p)&&p.revision==2&&p.valid,"revision");});
check("memory stale revision rejected",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0)&&!c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0),"cas");BankS6::MemoryRecord p;verify(c.nativeBank().readMemory(0,0,p)&&p.revision==1,"unchanged");});
check("patch stale revision rejected",[](){auto c=ready();verify(c.saveNativePatchForDiagnostic(0,0,7,patch(0),0)&&!c.saveNativePatchForDiagnostic(0,0,7,patch(0),0),"cas");});
check("config mismatch patch rejected",[](){auto c=ready();verify(c.saveNativePatchForDiagnostic(0,0,7,patch(0),0)&&!c.saveNativePatchForDiagnostic(0,0,8,patch(0),1),"no migration");});
check("config mismatch memory rejected",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0)&&!c.saveNativeMemoryForDiagnostic(0,0,8,memory(),1),"no migration");});
check("patch bad binary rejected",[](){auto c=ready();auto v=patch(0);v[16]=2;verify(!c.saveNativePatchForDiagnostic(0,0,7,v,0),"binary");});
check("patch bad mode low rejected",[](){auto c=ready();auto v=patch(0);v[28]=0;verify(!c.saveNativePatchForDiagnostic(0,0,7,v,0),"mode");});
check("patch bad mode high rejected",[](){auto c=ready();auto v=patch(12);v[41]=5;verify(!c.saveNativePatchForDiagnostic(12,0,7,v,0),"mode");});
check("patch upper second block binary",[](){auto c=ready();auto v=patch(8);v[42+18]=2;verify(!c.saveNativePatchForDiagnostic(8,0,7,v,0),"second binary");});
check("patch extra tail rejected",[](){auto c=ready();auto v=patch(0);v[40]=0;verify(!c.saveNativePatchForDiagnostic(0,0,7,v,0),"tail");});
check("patch no unknown in active cells",[](){auto c=ready();auto v=patch(8);v[0]=-2;verify(!c.saveNativePatchForDiagnostic(8,0,7,v,0),"unknown");});
check("patch config zero forbidden",[](){auto c=ready();verify(!c.saveNativePatchForDiagnostic(0,0,0,patch(0),0),"config");});
check("patch config 128 forbidden",[](){auto c=ready();verify(!c.saveNativePatchForDiagnostic(0,0,128,patch(0),0),"config");});
check("memory binary 6 rejected",[](){auto c=ready();auto v=memory();v[6]=2;verify(!c.saveNativeMemoryForDiagnostic(0,0,7,v,0),"binary");});
check("memory binary 8 rejected",[](){auto c=ready();auto v=memory();v[8]=3;verify(!c.saveNativeMemoryForDiagnostic(0,0,7,v,0),"binary");});
check("memory negative rejected",[](){auto c=ready();auto v=memory();v[0]=-2;verify(!c.saveNativeMemoryForDiagnostic(0,0,7,v,0),"unknown");});
check("memory >127 rejected",[](){auto c=ready();auto v=memory();v[5]=128;verify(!c.saveNativeMemoryForDiagnostic(0,0,7,v,0),"high");});
check("memory invalid slot rejected",[](){auto c=ready();verify(!c.saveNativeMemoryForDiagnostic(12,3,7,memory(),0),"invalid slot");});
check("memory config zero forbidden",[](){auto c=ready();verify(!c.saveNativeMemoryForDiagnostic(12,0,0,memory(),0),"config");});
check("memory no MIDI on write",[](){auto c=ready();c.enableTestMidiProjection(true);verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0)&&c.output().emitted()==0,"inert");});
check("patch no MIDI on write",[](){auto c=ready();c.enableTestMidiProjection(true);verify(c.saveNativePatchForDiagnostic(0,0,7,patch(0),0)&&c.output().emitted()==0,"inert");});
check("bank changes never change desired LIVE",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0)&&c.nativeLive().revision()==0,"no LIVE mutation");});
check("bank survives STOP in volatile engine",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0)&&c.onHostStop(),"stop");BankS6::MemoryRecord v;verify(c.nativeBank().readMemory(0,0,v)&&v.valid,"retained");});
check("bank memory snapshot reaches FX",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0),"write");FxEvent e{0,0,9,7,true,1};verify(c.configureFxOriginForDiagnostic(42)&&c.onFxEventForDiagnostic(42,e),"obs");verify(c.nativeFx().status(0).valid==1,"FX memory");});
check("bank mismatch compared only after 9 observations",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0)&&c.configureFxOriginForDiagnostic(42),"setup");for(int i=0;i<9;++i){auto v=memory();FxEvent e{0,i,v[i],7,true,static_cast<unsigned>(i+1)};verify(c.onFxEventForDiagnostic(42,e),"obs");}verify(c.nativeFx().status(c.nativeFx().pending()-4).equal==1,"MATCH");});
check("memory write after observation updates status",[](){auto c=ready();verify(c.configureFxOriginForDiagnostic(42),"origin");FxEvent e{0,0,9,7,true,1};verify(c.onFxEventForDiagnostic(42,e),"obs");c.drainFxStatusesForDiagnostic();verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0)&&c.nativeFx().pending()==1,"new status");verify(c.nativeFx().status(0).valid==1&&c.nativeFx().status(0).equal==-2,"status");});
check("memory save atomic when FX queue full",[](){auto c=ready();verify(c.configureFxOriginForDiagnostic(42),"origin");auto f=std::array<int,8>{125,88,75,50,7,17,7,6};for(int i=0;i<4;++i)verify(c.refreshFxStatusesForDiagnostic(42,f),"queue");auto revision=c.nativeBank().generation();verify(!c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0),"must reject");verify(c.nativeBank().generation()==revision,"bank unchanged");});
check("invalid input never changes FX queue",[](){auto c=ready();auto v=memory();v[7]=99;verify(!c.saveNativeMemoryForDiagnostic(0,0,7,v,0)&&c.nativeFx().pending()==0,"queue");});
check("PATCH not confused with MEMORY",[](){auto c=ready();verify(c.saveNativePatchForDiagnostic(0,0,7,patch(0),0),"patch");BankS6::MemoryRecord v;verify(c.nativeBank().readMemory(0,0,v)&&!v.valid,"not memory");});
check("MEMORY not confused with PATCH",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0),"memory");BankS6::PatchRecord v;verify(c.nativeBank().readPatch(0,0,v)&&!v.valid,"not patch");});
check("different slots separated",[](){auto c=ready();verify(c.saveNativeMemoryForDiagnostic(0,1,7,memory(),0),"save");BankS6::MemoryRecord a,b;verify(c.nativeBank().readMemory(0,0,a)&&c.nativeBank().readMemory(0,1,b)&&!a.valid&&b.valid,"slots");});
check("SAVE with no diagnostic guard blocked",[](){CoreCoordinator c;verify(!c.initializeEmptyNativeBanksForDiagnostic()&&!c.saveNativeMemoryForDiagnostic(0,0,7,memory(),0),"disabled");});
check("no fabricated session persistence",[](){auto c=ready();verify(c.nativeLive().snapshot().revision==0&&c.nativeBank().generation()==1,"independent");});
std::cout<<"PASS "<<count<<" L01/11 native BANKS scenarios\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
