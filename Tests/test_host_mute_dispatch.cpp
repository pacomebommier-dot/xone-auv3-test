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
 assert(e.mutePageVisible() && e.navMode()==0); // BOT2 did not also trigger legacy PRESET NAV
 input(e,36,true,100);input(e,36,false,199); // BASE 1 short
 assert(e.muteGesturePending()&&!e.globalMute().explicitlyMuted(0));
 assert(e.onMusicalGrid(bar())==1&&e.globalMute().explicitlyMuted(0));
 input(e,37,true,250);input(e,37,false,450); // BASE 2 long
 assert(e.onMusicalGrid(bar())==1&&e.globalMute().soloTarget()==1);
 input(e,13,true,500);input(e,15,true,550); // BOT2 then BOT4 combo
 input(e,15,false,560);input(e,13,false,570);
 assert(e.mutePageVisible()); // combo must not toggle BOT2
 // BOT2 as the SECOND control in a qualified BOT3+BOT2 combo is not a solo owner.
 input(e,14,true,575);input(e,13,true,580);input(e,13,false,585);input(e,14,false,590);
 assert(e.mutePageVisible());
 // A second BOT pressed AFTER the 90ms combo window cannot steal a true
 // BOT2 momentary overlay or prevent its restoration on the actual UP.
 input(e,13,true,600);e.advance(801,801*48);
 assert(!e.mutePageVisible());
 input(e,15,true,820);input(e,15,false,830);
 assert(!e.mutePageVisible());
 input(e,13,false,840);assert(e.mutePageVisible());
 input(e,36,true,900);e.stop(950);input(e,36,false,1000);
 assert(!e.muteGesturePending());
 // Two independently held BASE controls commit together, in release order.
 HostEngine multi;multi.enableNativeMutePage(true);
 input(multi,13,true,0);input(multi,13,false,10);
 input(multi,36,true,20);input(multi,37,true,21);
 input(multi,36,false,40);input(multi,37,false,41);
 assert(multi.globalMute().pendingCount()==2);
 assert(!multi.globalMute().explicitlyMuted(0)&&!multi.globalMute().explicitlyMuted(1));
 assert(multi.onMusicalGrid(bar())==1);
 assert(multi.globalMute().explicitlyMuted(0)&&multi.globalMute().explicitlyMuted(1));
 // A rejected late UP must neither arm nor release the physical capture.
 input(multi,38,true,100);input(multi,38,false,99);
 assert(!multi.muteGesturePending()&&multi.surfaceHeld(6));
 multi.stop(110*48);input(multi,38,false,120);
 assert(!multi.muteGesturePending()&&multi.onMusicalGrid(bar())==0);
 // In-buffer input can itself cross BOT2 LONG before the end-buffer timer.
 HostEngine sameEvent;sameEvent.enableNativeMutePage(true);
 input(sameEvent,13,true,0);input(sameEvent,36,true,200);
 assert(sameEvent.mutePageVisible());
 input(sameEvent,36,false,250);assert(sameEvent.muteGesturePending());
 input(sameEvent,13,false,260);assert(!sameEvent.mutePageVisible());
 assert(sameEvent.onMusicalGrid(bar())==1&&sameEvent.globalMute().explicitlyMuted(0));
 std::cout<<"PASS K2 BOT2 BASE host MUTE, BAR, S2 combo, STOP\n";
}
