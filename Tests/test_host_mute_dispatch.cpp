#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static Result input(HostEngine& e,int note,bool down,int ms) {
 uint8_t bytes[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};
 return e.receive(0,bytes,3,ms*48,ms);
}
static GridBatch bar(){GridBatch b{};b.valid=true;b.count=1;b.events[0]={GridKind::Bar,0,4};return b;}
int main(){
 HostEngine e;e.enableNativeMutePage(true);
 input(e,13,true,0);input(e,13,false,50); // short BOT2
 assert(e.mutePageVisible());
 input(e,36,true,100);input(e,36,false,199); // BASE 1 short
 assert(e.muteGesturePending()&&!e.globalMute().explicitlyMuted(0));
 assert(e.onMusicalGrid(bar())==1&&e.globalMute().explicitlyMuted(0));
 input(e,37,true,250);input(e,37,false,450); // BASE 2 long
 assert(e.onMusicalGrid(bar())==1&&e.globalMute().soloTarget()==1);
 input(e,13,true,500);input(e,15,true,550); // BOT2 then BOT4 combo
 input(e,15,false,560);input(e,13,false,570);
 assert(e.mutePageVisible()); // combo must not toggle BOT2
 // BOT2 as the SECOND control in a qualified BOT1+BOT2 combo is not a solo owner.
 input(e,12,true,575);input(e,13,true,580);input(e,13,false,585);input(e,12,false,590);
 assert(e.mutePageVisible());
 input(e,36,true,600);e.stop(650);input(e,36,false,700);
 assert(!e.muteGesturePending());
 std::cout<<"PASS K2 BOT2 BASE host MUTE, BAR, S2 combo, STOP\n";
}
