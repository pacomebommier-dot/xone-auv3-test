#include "../Shared/XoneBaseLedView.hpp"
#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <vector>
#include <iostream>
using namespace xone::host;
static void key(HostEngine& e,int note,bool down,int ms){
 const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};
 e.receive(0,b,3,ms*48,ms);
}
static std::vector<MuteLedOutput::Packet> flush(HostEngine& e,int ms){
 e.beginRenderBuffer();e.prepareMuteLeds(ms);unsigned cursor=0;std::vector<MuteLedOutput::Packet> tx;
 for(unsigned i=0;i<136;++i){auto p=e.nextMuteLedPacket(cursor);if(!p.valid)break;tx.push_back(p);e.ackMuteLedPacket(p,true);}
 return tx;
}
int main(){
 xone::l01::NavS3 nav;
 auto view=BaseLedView::compose(nav);for(auto p:view)assert(!p.known);
 assert(nav.initializeDefault());view=BaseLedView::compose(nav);
 for(auto p:view)assert(p.known&&p.color==MuteLedColor::Off&&!p.blinking);
 auto selected=nav.active();selected.at(293)=1;selected.at(299)=1;
 assert(xone::l01::NavS3::regenerate(selected));assert(nav.stage(selected)&&nav.commit());
 view=BaseLedView::compose(nav);assert(view[0].known&&view[0].color==MuteLedColor::Red);
 assert(view[1].color==MuteLedColor::Off);
 // S7 @x4 MACHINE selected synth/drum/sample colors.
 assert(nav.applyShortMode(7));view=BaseLedView::compose(nav);
 bool red=false;for(auto p:view)red|=p.known&&p.color==MuteLedColor::Red;
 assert(red); // source @x4: default d314=2 is red (1..4), 5..8 yellow, 9..16 green
 assert(nav.applyShortMode(4));view=BaseLedView::compose(nav);
 bool unknown=false;for(auto p:view)unknown|=!p.known;assert(unknown); // unresolved FX/MEMORY owners
 HostEngine e;e.enableNativeMutePage(true);
 key(e,36,true,0);key(e,36,false,50);auto initial=flush(e,50);assert(initial.size()==101); // 100 proven OFFs + selected red BASE1
 unsigned lit=0;for(auto p:initial)if(p.on){assert(p.target==0&&p.bytes[1]==36);++lit;}assert(lit==1);
 key(e,13,true,60);key(e,13,false,70);assert(flush(e,70).size()==17); // cache already known: old red OFF + 16 green ON
 key(e,32,true,80);key(e,32,false,90);assert(e.muteGesturePending()); // MUTE must not select another underlying source
 key(e,13,true,100);key(e,13,false,110);
 auto tx=flush(e,110);bool restored=false;
 for(auto p:tx)if(p.on){assert(p.target==0&&p.bytes[0]==0x9E&&p.bytes[1]==36);restored=true;}
 assert(restored&&!e.mutePageVisible());assert(flush(e,111).empty());
 // Known underlying selection continues updating after exit.
 key(e,32,true,120);key(e,32,false,170);tx=flush(e,170);
 bool oldOff=false,newOn=false;
 for(auto p:tx){oldOff|=!p.on&&p.bytes[1]==36;newOn|=p.on&&p.bytes[1]==32;}
 assert(oldOff&&newOn);
 // DOWN in a momentary MUTE visit keeps ownership after the page restores.
 key(e,13,true,200);e.advance(400,400*48);assert(e.mutePageVisible());flush(e,400);
 e.advance(500,500*48);flush(e,500); // pending LED is in the OFF blink phase
 key(e,38,true,510);key(e,13,false,520);assert(!e.mutePageVisible());
 key(e,38,false,530);assert(e.globalMute().pendingFor(2).valid);
 tx=flush(e,530);bool restoredSelection=false;
 for(auto p:tx)if(p.on){assert(p.target==4&&p.bytes[1]==32);restoredSelection=true;}
 assert(restoredSelection);
 // Callback failure during restoration retries its purge before the context ON.
 key(e,13,true,600);key(e,13,false,610);flush(e,610);
 key(e,13,true,620);key(e,13,false,630);e.beginRenderBuffer();e.prepareMuteLeds(630);
 unsigned cursor=0;auto failed=e.nextMuteLedPacket(cursor);assert(failed.valid);
 e.ackMuteLedPacket(failed,false);tx=flush(e,631);bool retried=false;
 for(auto p:tx)if(p.target==failed.target){assert(!p.on);retried=true;break;}
 assert(retried&&flush(e,632).empty());
 // Match the real AU sequence: selected red BASE2, three BAR changes,
 // cancelled BASE2 at STOP, BASE3 applied at a BAR/STOP tie, then exit.
 HostEngine au;au.enableNativeMutePage(true);
 key(au,37,true,0);key(au,37,false,1);
 key(au,13,true,2);key(au,13,false,3);flush(au,3);
 GridBatch edge{};edge.valid=true;edge.count=1;edge.events[0]={GridKind::Bar,0,4};
 for(int n=0;n<3;++n){key(au,36,true,4+2000*n);key(au,36,false,5+2000*n);au.onMusicalGrid(edge);flush(au,2000+2000*n);}
 key(au,37,true,7998);au.onMusicalGrid(edge);key(au,37,false,8000);
 au.stop(9999*48);au.onMusicalGrid(edge);flush(au,10000);
 key(au,38,true,10001);key(au,38,false,10002);au.onMusicalGrid(edge);au.stop(12000*48);flush(au,12000);
 key(au,13,true,12001);key(au,13,false,12002);tx=flush(au,12002);
 bool redRestored=false;
 for(auto p:tx)if(p.on){assert(p.bytes[1]==37);redRestored=true;}
 assert(redRestored);
 std::cout<<"PASS S7 BASE context selection, UNKNOWN, MUTE restoration and source ownership\n";
}