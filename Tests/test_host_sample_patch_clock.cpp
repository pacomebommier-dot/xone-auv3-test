#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void edge(HostEngine& e,int note,bool down,int ms){const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};e.receive(0,b,3,ms*48,ms);}
int main(){
 HostEngine e;xone::l01::LiveState::SessionImage image{};image.schema=1;image.config=7;image.revision=1;image.desired.fill(-2);assert(e.restoreSessionBeforeRendering(image));
 xone::l01::BankS6::PatchImage patch{};patch.fill(-2);for(int j=0;j<40;++j)patch[j]=j<16?30:j<28?0:1;
 assert(e.storeSamplePatchBeforeRendering(0,0,7,patch,0));
 edge(e,13,true,10);edge(e,14,true,20);edge(e,14,false,30);edge(e,13,false,40);
 HostGrid host;MusicalContext c{};c.available=true;c.moving=true;c.tempo=120;c.numerator=4;c.denominator=4;c.sampleRate=48000;c.beatPosition=7;c.measureDownbeat=4;
 auto b=host.scan(c,48);assert(b.valid&&b.count==0);e.samplePatchOnGrid(b,100);
 c.beatPosition=8;c.measureDownbeat=8;b=host.scan(c,48);assert(b.count==1&&b.events[0].kind==GridKind::Bar);e.samplePatchOnGrid(b,200);
 edge(e,52,true,50);edge(e,52,false,60);assert(e.samplePatchPending());
 c.beatPosition=3;c.measureDownbeat=0;b=host.scan(c,48);assert(b.valid&&b.count==0);assert(!e.samplePatchOnGrid(b,300).emit&&e.samplePatchPending());
 c.beatPosition=4;c.measureDownbeat=4;b=host.scan(c,48);assert(b.count==1&&b.events[0].kind==GridKind::Bar);
 auto r=e.samplePatchOnGrid(b,400);assert(r.packetCount()==28&&!e.samplePatchPending());
 std::cout<<"PASS SAMPLE recall follows the first genuine BAR after host seek, never the fresh anchor\n";
}
