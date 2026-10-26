#include "../Core/NavS3.hpp"
#include <stdexcept>
#include <iostream>
#include <array>
#include <string>
#include <vector>
using namespace xone::l01;
static unsigned tests=0;
static void req(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
static void run(const char*name,void(*fn)()){fn();++tests;std::cout<<"PASS "<<name<<"\n";}
static NavS3 baseline(){NavS3 n;req(n.initializeDefault(),"initialize default");return n;}
static NavImage edited(int index,int value){auto n=baseline().active();n.at(index)=value;req(NavS3::regenerate(n),"regenerate editor");return n;}
static bool committed(NavS3& n,const NavImage&i){req(n.stage(i),"stage");return n.commit();}
int main(){try{
run("NAV-SOURCE-DEFAULT-133",[](){auto n=baseline();req(n.stable()&&n.revision()==1&&NavS3::validate(n.active()),"default");req(n.active().at(291)==0&&n.active().at(292)==0&&n.active().contextMode==0,"header");});
run("NAV-52-AFFECTATIONS-RECALCULEES",[](){auto n=baseline();for(int j=4;j<56;++j){auto r=n.active().route(j);req(r.address!=-2&&r.position!=-2,"route unknown");}req(n.active().route(4)==NavRoute{2145,0},"j4 literal @zd");req(n.active().route(7)==NavRoute{2213,0},"j7 literal @zd");req(n.active().route(20)==NavRoute{9*1024+7*32,0},"j20 literal");});
run("NAV-STAGED-DOES-NOT-AFFECT-LIVE",[](){auto n=baseline();auto orig=n.active();auto proposal=edited(313,2);req(n.stage(proposal),"stage");req(n.active()==orig&&n.revision()==1,"live mutated by staging");req(n.hasStage(),"missing stage");n.abandon();req(!n.hasStage()&&n.active()==orig,"abandon mutated");});
run("NAV-VALID-COMMIT-COPIES-WHOLE-IMAGE",[](){auto n=baseline();auto m=n.active();m.at(313)=7;req(NavS3::regenerate(m)&&committed(n,m),"commit");req(n.active()==m&&n.revision()==2,"not atomic");req(n.undoSnapshot().at(313)==1,"undo copy");});
run("NAV-TAMPER-ROUTE-IS-REJECTED",[](){auto n=baseline();auto old=n.active();auto m=old;m.at(320)=123456;req(n.stage(m)&&!n.commit(),"tampered route accepted");req(n.active()==old&&n.revision()==1,"tamper modified active");});
run("NAV-TAMPER-LAST-ROUTE-IS-REJECTED",[](){auto n=baseline();auto m=n.active();m.at(423)=-2;req(n.stage(m)&&!n.commit()&&!(n.active().route(55)==NavRoute{-2,-2}),"last mapping changed");});
run("NAV-CORRUPT-BYTE-ROLLED-BACK",[](){auto n=baseline();auto old=n.active();auto m=old;m.at(314)=25;req(n.stage(m)&&!n.commit()&&n.active()==old&&n.revision()==1,"invalid nav committed");});
run("NAV-INVALID-PAGE-ROLLED-BACK",[](){auto n=baseline();auto old=n.active();auto m=old;m.at(291)=2;req(n.stage(m)&&!n.commit()&&n.active()==old,"page committed");});
run("NAV-INVALID-PAIR-PRIORITY",[](){auto n=baseline();auto m=n.active();m.at(309)=1;m.at(310)=1;req(NavS3::regenerate(m)&&n.stage(m)&&!n.commit(),"priority duplicates accepted");});
run("NAV-INVALID-COLUMN-ID",[](){auto n=baseline();auto m=n.active();m.at(318)=3;req(NavS3::regenerate(m)&&n.stage(m)&&!n.commit(),"fixed column invalid");});
run("NAV-UNAUTHORISED-CONTEXT",[](){auto n=baseline();auto m=n.active();m.contextMode=6;req(n.stage(m)&&!n.commit(),"context 6 accepted");});
run("NAV-MODE-CONTEXT-MISMATCH",[](){auto n=baseline();auto m=n.active();m.at(292)=3;req(NavS3::regenerate(m)&&n.stage(m)&&!n.commit(),"mode3 without context accepted");});
run("NAV-PRESET-FLAG-MISMATCH",[](){auto n=baseline();auto m=n.active();m.at(292)=1;req(NavS3::regenerate(m)&&n.stage(m)&&!n.commit(),"mode1 without preset flag accepted");});
run("NAV-RED-PAGE-TOGGLE",[](){auto n=baseline();req(n.applyShortMode(8),"toggle");req(n.active().at(291)==1&&n.active().at(292)==0&&NavS3::validate(n.active()),"toggle page");req(n.applyShortMode(8)&&n.active().at(291)==0,"toggle back");});
run("NAV-RETURN-BASE-SOURCE-ZJ",[](){auto n=baseline();req(n.applyShortMode(3),"enter mode3");req(n.active().at(292)==3&&n.active().contextMode==3,"mode3");req(n.applyShortMode(0),"base");req(n.active().at(292)==0&&n.active().contextMode==0&&n.active().at(291)==0,"return BASE");});
run("NAV-TOGGLE-SAME-MODE-SOURCE-ZJ",[](){auto n=baseline();req(n.applyShortMode(2)&&n.active().at(292)==2,"mode2");req(n.applyShortMode(2)&&n.active().at(292)==0,"toggle off");});
run("NAV-ALL-MODE-SELECTIONS-NO-FABRICATION",[](){for(int mode=1;mode<=7;++mode){auto n=baseline();req(n.applyShortMode(mode),"select mode");req(n.active().at(292)==mode&&NavS3::validate(n.active()),"mode invalid");if(mode==1)req(n.active().presetFlag==1,"preset flag");if(mode!=5&&mode!=6&&mode!=1)req(n.active().contextMode==mode,"mode context");}});
run("NAV-CONTEXT-BROWSER-PHYSICAL-MAPPING",[](){auto n=baseline();req(n.applyShortMode(3),"browse");const auto a=n.active().route(4);req(a.address==2*1024+32+1 && a.position==0,"browser physical 4");req(n.active().route(7).address==-1,"browser physical 12 forbidden @zn");});
run("NAV-CONTEXT-MACHINE-PHYSICAL-MAPPING",[](){auto n=baseline();req(n.applyShortMode(7),"machine");req(n.active().route(4).address==2*1024+2*32+1,"machine first");});
run("NAV-PRESET-OVERRIDE-16",[](){auto n=baseline();req(n.applyShortMode(1),"preset");for(int j=4;j<20;++j)req(n.active().route(j)==NavRoute{14*1024+17*32,j-4},"preset map");});
run("NAV-MODE-SIX-SLOT-IS-CAPTURED",[](){auto n=baseline();auto m=n.active();m.at(292)=6;m.sampleSlot=2;req(NavS3::regenerate(m)&&committed(n,m),"mode 6");req(n.active().route(8).address==26*1024+64,"sample lane ignored");m=n.active();m.sampleSlot=3;req(NavS3::regenerate(m)&&committed(n,m),"sample switched");req(n.active().route(8).address==26*1024+96,"sample lane stale");});
run("NAV-SLOT-OUT-OF-RANGE-REFUSED",[](){auto n=baseline();auto old=n.active();auto m=old;m.sampleSlot=-2;req(!NavS3::regenerate(m),"unknown lane fabricated");req(n.stage(m)&&!n.commit()&&n.active()==old,"unknown lane committed");});
run("NAV-MODE-FIVE-MAPPING-RECURSIVE",[](){auto n=baseline();req(n.applyShortMode(5),"mode5");for(int j=20;j<32;++j){const auto r=n.active().route(j);req(r.address!=-2&&r.position!=-2,"recursive mapping not generated");}});
run("NAV-TWO-BANKS-INDEPENDENT",[](){auto n=baseline();auto a=n.active();a.at(293)=2;a.at(299)=1;req(NavS3::regenerate(a)&&committed(n,a),"bank0");auto b=n.active();b.at(291)=1;b.at(295)=2;b.at(301)=1;req(NavS3::regenerate(b)&&committed(n,b),"bank1");req(n.active().at(293)==2&&n.active().at(295)==2&&n.active().at(291)==1,"bank leakage");});
run("NAV-STAGE-HEADER-DERIVES-104-CELLS",[](){auto n=baseline();std::array<int,29> h{};for(std::size_t i=0;i<29;++i)h[i]=n.active().cells[i];h[1]=3;h[22]=6;req(n.stageHeader(h,3,0),"stageHeader");req(n.stageSnapshot().route(36).address==32+6,"route not generated");req(n.commit()&&n.active().at(313)==6,"header commit");});
run("NAV-EXHAUSTIVE-52-EACH-PAGE",[](){auto n=baseline();for(int mode=0;mode<8;++mode){if(mode>0)req(n.applyShortMode(mode),"mode");for(int page=0;page<2;++page){auto m=n.active();m.at(291)=page;req(NavS3::regenerate(m)&&committed(n,m),"regen commit");for(int j=4;j<56;++j)req(n.active().route(j).address!=-2&&n.active().route(j).position!=-2,"52 route invalid");}}});
run("NAV-INVALID-REGEN-NO-STAGE",[](){auto n=baseline();auto old=n.active();std::array<int,29> h{};for(std::size_t i=0;i<29;++i)h[i]=old.cells[i];h[0]=-1;req(!n.stageHeader(h,0,0),"invalid rebuild staged");req(!n.hasStage()&&n.active()==old,"invalid touched active");});
run("NAV-STAGE-ABANDON-RETAIN-REVISION",[](){auto n=baseline();auto m=edited(313,3);req(n.stage(m),"stage");n.abandon();req(n.revision()==1&&!n.hasStage(),"revision changed");});
run("NAV-UNSAFE-DIRECT-BAD-ROUTE-REJECTED",[](){auto n=baseline();auto m=n.active();m.at(370)= -2;req(n.stage(m)&&!n.commit(),"unknown route accepted");});
run("NAV-BOUNDARY-BAD-H7-SAFETY",[](){auto n=baseline();auto old=n.active();req(!n.applyShortMode(-1)&&!n.applyShortMode(9),"invalid mode");req(n.active()==old,"invalid mutated");});
run("NAV-PENDING-STAGE-BLOCKS-MODE",[](){auto n=baseline();req(n.stage(n.active()),"stage");req(!n.applyShortMode(8),"mode stole pending stage");req(n.active().at(291)==0,"pending altered page");});
run("NAV-ALL-FIELDS-NONCURRENT-PROTECTED",[](){auto n=baseline();auto old=n.active();for(int i=320;i<424;++i){auto m=old;m.at(i)=i;req(n.stage(m),"stage");req(!n.commit(),"tamper accepted");req(n.active()==old,"tamper applied");n.abandon();}});
run("NAV-SI-ROLE-TABLE-SOURCE",[](){const std::array<int,11> source{-1,1,2,8,3,4,7,5,6,0,-1};for(int role=0;role<11;++role)req(NavS3::shortRoleMode(role)==source[static_cast<std::size_t>(role)],"@si role mapping");});
run("NAV-ALL-52-ROUTE-PAIR-STORAGE-POSITIONS",[](){auto n=baseline();for(int j=4;j<56;++j){int off=320+2*(j-4);req(n.active().route(j).address==n.active().at(off),"route address offset");req(n.active().route(j).position==n.active().at(off+1),"route pos offset");}});
std::cout<<"PASS "<<tests<<" NAV S3 stage/validate/commit scenarios\n";return 0;
}catch(const std::exception&ex){std::cerr<<"FAIL "<<ex.what()<<"\n";return 1;}}
