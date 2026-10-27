#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void edge(HostEngine& e,int note,bool down,int ms){const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};assert(!e.receive(0,b,3,ms*48,ms).emit);}
static HostEngine::TopIntent top(HostEngine& e,int index,int ms){edge(e,52+index,true,ms);edge(e,52+index,false,ms+1);HostEngine::TopIntent out{};assert(e.lastTopIntent(index,out));return out;}
int main(){
 HostEngine e;
 auto normal=top(e,0,1);assert(normal.action==HostEngine::TopAction::FilterReset&&normal.route.position==0);
 const auto global=top(e,3,3).route;
 edge(e,15,true,10);edge(e,15,false,20);
 auto yellow=top(e,0,30);assert(yellow.action==HostEngine::TopAction::FilterReset&&yellow.route.position==2);
 assert(top(e,3,32).route==global);
 edge(e,13,true,40);edge(e,14,true,50);edge(e,14,false,60);edge(e,13,false,70);assert(e.navMode()==3);
 auto sample=top(e,0,80);assert(sample.action==HostEngine::TopAction::PatchRecall&&sample.route.position==0);
 assert(top(e,3,90).route==global);
 edge(e,13,true,100);edge(e,13,false,110);assert(e.navMode()==1);
 auto preset=top(e,0,120);assert(preset.action==HostEngine::TopAction::PatchRecall&&preset.route==sample.route);
 assert(top(e,3,130).route==global);
 edge(e,14,true,140);edge(e,15,true,150);edge(e,15,false,160);edge(e,14,false,170);assert(e.navMode()==5);
 auto reset=top(e,0,180);assert(reset.action==HostEngine::TopAction::PatchRecall&&reset.route==sample.route);
 assert(top(e,3,190).route==global);
 assert(!e.desiredLive(1476).known); // no invented FILTER GLOBAL reset/default
 e.stop(200*48);HostEngine::TopIntent out{};assert(!e.lastTopIntent(0,out));
 // Exit RESET then enter MACHINE with its real BOT2 -> BOT4 combo.
 edge(e,14,true,220);edge(e,15,true,230);edge(e,15,false,240);edge(e,14,false,250);assert(e.navMode()==0);
 edge(e,13,true,260);edge(e,15,true,270);edge(e,15,false,280);edge(e,13,false,290);assert(e.navMode()==7);
 for(int i=0;i<3;++i){auto machine=top(e,i,300+2*i);assert(machine.action==HostEngine::TopAction::PatchRecall&&machine.route.position==i);}
 assert(top(e,3,310).route==global);
 std::cout<<"PASS TOP contextual NORMAL/YELLOW, SAMPLE PATCH through PRESET/RESET, invariant GLOBAL and UNKNOWN\n";
}
