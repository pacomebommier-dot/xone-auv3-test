#include "../Shared/XoneHostEngine.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <new>
#include <cassert>
#include <iostream>
static bool measureAllocations=false;
static std::size_t allocationCount=0;
#if defined(__GNUC__) || defined(__clang__)
#define TEST_NOINLINE __attribute__((noinline))
#else
#define TEST_NOINLINE
#endif
TEST_NOINLINE void* operator new(std::size_t size){if(measureAllocations)++allocationCount;if(void* p=std::malloc(size?size:1))return p;throw std::bad_alloc();}
TEST_NOINLINE void* operator new[](std::size_t size){return ::operator new(size);}
TEST_NOINLINE void operator delete(void* p)noexcept{std::free(p);}
TEST_NOINLINE void operator delete[](void* p)noexcept{std::free(p);}
TEST_NOINLINE void operator delete(void* p,std::size_t)noexcept{std::free(p);}
TEST_NOINLINE void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
using namespace xone::host;
int main(){
 HostEngine engine;engine.enableNativeMutePage(true);engine.observeTransport(true);HostGrid grid;
 using Clock=std::chrono::steady_clock;const auto warmClock=Clock::now();(void)warmClock;
 constexpr unsigned Blocks=4096,Frames=128;std::array<double,Blocks> elapsed{};
 unsigned music=0,led=0,failed=0,sourceAttempts=0;int64_t sample=0;
 const auto sent=[&](const Result& r){for(unsigned i=0;i<r.packetCount();++i){auto p=r.packet(i);if(!engine.canSend(p))continue;assert((p.bytes[0]&15)>=6&&(p.bytes[0]&15)<=13);engine.sent(p,0);++music;}};
 const auto key=[&](int note,bool down,int64_t ms){const uint8_t d[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};sent(engine.receive(0,d,3,sample,ms));};
 measureAllocations=true;
 for(unsigned block=0;block<Blocks;++block){
  const auto start=Clock::now();const int64_t ms=sample*1000/48000;
  if(block%257==0)engine.requestPresetPage((block/257)%2!=0);
  engine.beginRenderBuffer();engine.beginSourceRetryPass();engine.beginCueLoadRetryPass();engine.beginStopRetryPass(sample);
  MusicalContext context{120,4,static_cast<double>(sample)/24000.0,0,48000,4,((sample+23999)/24000)*24000-sample,true,true};
  engine.onMusicalGrid(grid.scan(context,Frames));
  const uint8_t cc[]{0xBE,16,static_cast<uint8_t>(block%128)};sent(engine.receive(0,cc,3,sample,ms));
  if(block%97==0)for(unsigned source=0;source<4;++source)engine.requestSourceRecall(source,3);
  if(block%31==0){key(13,true,ms);key(13,false,ms);}
  if(block%43==0){key(36,true,ms);key(36,false,ms);}
  if(block%47==0){key(48,true,ms);key(48,false,ms);}
  if(block%61==0){key(12,true,ms);key(12,false,ms);}
  for(unsigned i=0;i<2048;++i){auto p=engine.nextSourcePacket();if(!p.valid)break;const bool success=++sourceAttempts%113!=0;engine.ackSourcePacket(p,success,sample);++music;if(!success){++failed;break;}}
  unsigned cursor=0;for(unsigned i=0;i<16;++i){auto p=engine.nextMutePacket(cursor);if(!p.valid)break;engine.ackMutePacket(p.target,true);++music;}
  engine.advance(ms+2,sample+Frames);engine.prepareMuteLeds(ms+2);cursor=0;
  for(unsigned i=0;i<136;++i){auto p=engine.nextMuteLedPacket(cursor);if(!p.valid)break;assert((p.bytes[0]&15)==14);engine.ackMuteLedPacket(p,true);++led;}
  engine.endRenderBuffer();sample+=Frames;
  elapsed[block]=std::chrono::duration<double,std::micro>(Clock::now()-start).count();
 }
 engine.stop(sample);engine.beginStopRetryPass(sample);
 for(unsigned i=0;i<1024;++i){auto p=engine.nextStopPacket();if(!p.emit)break;engine.sent(p,0);++music;}
 assert(!engine.nextStopPacket().emit);
 for(unsigned keyIndex=0;keyIndex<36;++keyIndex)assert(!engine.surfaceHeld(keyIndex));
 measureAllocations=false;
 if(engine.overflows()||allocationCount||failed==0)std::cerr<<"overflow="<<engine.overflows()<<" allocations="<<allocationCount<<" failed="<<failed<<" music="<<music<<" LED="<<led<<"\n";
 assert(engine.overflows()==0&&engine.observed()==-2&&allocationCount==0&&music>100&&led>100&&failed>0);
 std::sort(elapsed.begin(),elapsed.end());double sum=0;for(double value:elapsed)sum+=value;
 std::cout<<"PASS native render stress blocks="<<Blocks<<" frames="<<Frames<<" C++_allocations="<<allocationCount<<" music="<<music<<" LED="<<led<<" injected_failures="<<failed<<" mean_us="<<sum/Blocks<<" p99_us="<<elapsed[Blocks*99/100]<<" max_us="<<elapsed.back()<<"; software measurement, not AUM/iPhone CPU or realtime certification\n";
}
