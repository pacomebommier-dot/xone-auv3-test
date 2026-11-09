#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void edge(HostEngine& e,int note,bool down,int ms){const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};assert(!e.receive(0,b,3,ms*48,ms).emit);}
static HostEngine::TopIntent top(HostEngine& e,int index,int ms){edge(e,52+index,true,ms);edge(e,52+index,false,ms+1);HostEngine::TopIntent out{};assert(e.lastTopIntent(index,out));return out;}
int main(){
 HostEngine e;e.enableNativeMutePage(true);
 // Bare INIT TOP1 is a transient ACTION pulse, not a FILTER RESET.
 edge(e,52,true,1);edge(e,52,false,2);
 assert(e.pendingTopPulses()==1);
 HostEngine::TopIntent none{};assert(!e.lastTopIntent(0,none));
 const auto global=top(e,3,3).route;
 edge(e,15,true,10);edge(e,15,false,20);
 // Root1 TOP1 is YELLOW INIT, never FILTER RESET.
 edge(e,52,true,30);edge(e,52,false,31);
 assert(e.pendingTopPulses()==2);
 auto yellow=e.nextTopPulse();assert(yellow.valid&&yellow.api==680);
 assert(top(e,3,32).route==global);
 edge(e,13,true,40);edge(e,14,true,50);edge(e,14,false,60);edge(e,13,false,70);assert(e.navMode()==3);
 auto sample=top(e,0,80);assert(sample.action==HostEngine::TopAction::PatchRecall&&sample.route.position==0);
 assert(top(e,3,90).route==global);
 // SOC-06: BOT2 short is the MUTE toggle even while SAMPLE is underneath.
 edge(e,13,true,100);edge(e,13,false,110);assert(e.navMode()==3&&e.mutePageVisible());
 edge(e,13,true,111);edge(e,13,false,112);assert(e.navMode()==3&&!e.mutePageVisible());
 auto sampleAgain=top(e,0,120);assert(sampleAgain.action==HostEngine::TopAction::PatchRecall&&sampleAgain.route==sample.route);
 assert(top(e,3,130).route==global);
 edge(e,14,true,140);edge(e,15,true,150);edge(e,15,false,160);edge(e,14,false,170);assert(e.navMode()==5);
 auto reset=top(e,0,180);assert(reset.action==HostEngine::TopAction::PatchRecall&&reset.route==sample.route);
 assert(top(e,3,190).route==global);
 assert(!e.desiredLive(1476).known); // no invented FILTER GLOBAL reset/default
 e.stop(200*48);HostEngine::TopIntent out{};assert(!e.lastTopIntent(0,out));
 // Exit RESET then enter MACHINE with its real BOT2 -> BOT4 combo.
 edge(e,14,true,220);edge(e,15,true,230);edge(e,15,false,240);edge(e,14,false,250);assert(e.navMode()==0);
 edge(e,13,true,260);edge(e,15,true,270);edge(e,15,false,280);edge(e,13,false,290);assert(e.navMode()==7);
 // SOC-06: an empty MACHINE slot SHORT captures at true UP;
 // UNKNOWN LIVE refuses without a deferred recall or saved image.
 for(int i=0;i<3;++i){
  edge(e,52+i,true,300+2*i);edge(e,52+i,false,301+2*i);
  SourcePatchBank::Patch patch{};
  assert(!e.lastTopIntent(static_cast<unsigned>(i),out)&&!e.samplePatchPending());
  for(unsigned source=0;source<4;++source)assert(!e.readSourcePatch(source,static_cast<unsigned>(i),patch));
 }
 assert(top(e,3,310).route==global);
 std::cout<<"PASS TOP contextual NORMAL/YELLOW, SAMPLE PATCH through PRESET/RESET, invariant GLOBAL and UNKNOWN\n";
}
