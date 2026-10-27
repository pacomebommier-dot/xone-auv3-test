#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static SessionImage image(){SessionImage i{};i.schema=1;i.config=1;i.revision=1;i.desired.fill(-2);for(int c=1477;c<1513;++c)i.desired[c]=0;for(int c=1768;c<1864;++c)i.desired[c]=0;return i;}
static Result edge(HostEngine& e,int note,bool down,int ms){uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};return e.receive(0,b,3,ms,ms);}
int main(){
 HostEngine e;assert(e.restoreSessionBeforeRendering(image()));
 edge(e,36,true,0);edge(e,36,false,0);auto r=edge(e,48,true,1);assert(r.emit && (r.bytes[0]&0xF0)==0x90);const int a=r.api;e.sent(r,0);
 r=edge(e,48,false,199);assert(!r.emit); // SMART short latches
 r=edge(e,48,true,300);assert(!r.emit);r=edge(e,48,false,310);assert(r.emit&&r.api==a&&(r.bytes[0]&0xF0)==0x80);e.sent(r,-1);
 e.stop(320);r=e.nextStopPacket();assert(r.emit&&r.api==a&&(r.bytes[0]&0xF0)==0x80);e.sent(r,0);assert(!e.nextStopPacket().emit);
 r=edge(e,48,true,400);assert(r.emit);e.sent(r,0);r=edge(e,48,false,600);assert(r.emit&&(r.bytes[0]&0xF0)==0x80);e.sent(r,0);
 r=edge(e,48,true,700);assert(r.emit);e.sent(r,0);e.stop(710);r=e.nextStopPacket();assert(r.emit);e.sent(r,0);assert(e.surfaceHeld(20));assert(!edge(e,48,false,720).emit);
 // Destination stays captured across another physical FX selection.
 r=edge(e,48,true,800);assert(r.emit);e.sent(r,0);
 edge(e,32,true,810);edge(e,32,false,820);
 r=edge(e,48,false,1000);assert(r.emit&&r.api==a);e.sent(r,0);
 // A callback failure on ON is conservatively treated as a possible sent note.
 r=edge(e,48,true,1100);assert(r.emit);e.sent(r,-1);
 e.stop(1110);r=e.nextStopPacket();assert(r.emit);e.sent(r,-1);
 e.stop(1120);r=e.nextStopPacket();assert(r.emit);e.sent(r,0);
 assert(!e.nextStopPacket().emit);assert(!edge(e,48,false,1130).emit);
 HostEngine unknown;assert(!edge(unknown,48,true,0).emit);assert(!edge(unknown,48,false,1).emit);
 // Several sounding SPOT notes are all closed; only the failed OFF remains due.
 HostEngine batch;assert(batch.restoreSessionBeforeRendering(image()));edge(batch,36,true,0);edge(batch,36,false,1);
 const int buttons[]{48,44,40};
 for(int n:buttons){auto out=edge(batch,n,true,10);assert(out.emit);batch.sent(out,0);}
 batch.stop(20);
 for(int n=0;n<3;++n){auto out=batch.nextStopPacket();assert(out.emit&&out.api==384+n);batch.sent(out,n==1?-1:0);}
 assert(!batch.nextStopPacket().emit);batch.stop(30);auto retry=batch.nextStopPacket();assert(retry.emit&&retry.api==385);batch.sent(retry,0);assert(!batch.nextStopPacket().emit);
 for(int n:buttons)assert(!edge(batch,n,false,40).emit);
 // Revision exhaustion must never suppress a physically owed NoteOff.
 HostEngine exhausted;auto limit=image();limit.revision=xone::l01::LiveState::MaxRevision-1;
 assert(exhausted.restoreSessionBeforeRendering(limit));edge(exhausted,36,true,0);edge(exhausted,36,false,1);
 auto last=edge(exhausted,48,true,10);assert(last.emit);exhausted.sent(last,0);
 auto close=edge(exhausted,48,false,210);assert(close.emit&&close.value==0);exhausted.sent(close,0);
 assert(exhausted.desiredLive(1768).value==1); // failed LIVE write is not claimed committed
 exhausted.stop(220);assert(!exhausted.nextStopPacket().emit);
 // MACHINE NAV uses the genuine BOT2+BOT4 combo, with explicit known session fields.
 for(int mode:{-2,0,1,2,5,127}){
  HostEngine m;auto i=image();i.desired[380]=0;i.desired[392]=mode;assert(m.restoreSessionBeforeRendering(i));
  edge(m,13,true,0);edge(m,15,true,10);edge(m,15,false,20);edge(m,13,false,30);assert(m.navMode()==7);
  auto on=edge(m,48,true,40);
  if(mode<1||mode>2){assert(!on.emit&&!edge(m,48,false,400).emit);assert(m.desiredLive(380).value==0);continue;}
  assert(on.emit&&on.api==108&&on.bytes[0]==0x96);m.sent(on,0);
  auto up=edge(m,48,false,240);
  if(mode==1){assert(up.emit&&up.value==0);m.sent(up,0);}
  else{assert(!up.emit&&m.desiredLive(380).value==1);edge(m,48,true,300);up=edge(m,48,false,500);assert(up.emit&&up.value==0);m.sent(up,0);}
 }
 std::cout<<"PASS SMART/MIX musical output, captured destination, 199/200ms, failed ON/OFF debts, multi-note STOP, UNKNOWN\n";
}


