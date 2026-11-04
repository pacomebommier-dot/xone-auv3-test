#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static int note(int p){for(int n=24;n<40;++n)if(xone::l01::SurfaceInput::physicalControl(n)==p)return n;assert(false);return -1;}
static int selector(const xone::l01::NavImage& n,int machine){for(int p=4;p<20;++p)if(n.route(p).address==2*1024+2*32+machine)return p;assert(false);return -1;}
static void edge(HostEngine& h,int p,bool down,int ms){const uint8_t b[]{uint8_t(down?0x9e:0x8e),uint8_t(note(p)),uint8_t(down?127:0)};assert(!h.receive(0,b,3,ms,ms).emit);}
static void bot(HostEngine& h,int n,bool down,int ms){const uint8_t b[]{uint8_t(down?0x9E:0x8E),uint8_t(n),uint8_t(down?127:0)};assert(!h.receive(0,b,3,ms,ms).emit);}
int main(){
 for(int machine:{2,4,6,8}){HostEngine h;xone::l01::NavS3 nav;assert(nav.initializeDefault());auto n=nav.active();n.at(292)=7;n.contextMode=7;n.at(314)=machine;
  assert(xone::l01::NavS3::regenerate(n)&&h.setNavForDiagnostic(n));const int key=selector(n,machine),source=(machine-1)/2;
  SessionImage s{};s.schema=1;s.config=1;s.revision=1;s.desired.fill(-2);for(int c=1552;c<1648;++c)s.desired[c]=64;
  assert(h.restoreSessionBeforeRendering(s));
  assert(!h.machineFxBorrowed()&&continuousTarget(h.routeForControl(40)).cell==1552+24*source);
  edge(h,key,true,10);h.advance(209,209);assert(!h.machineFxBorrowed());h.advance(210,210);assert(h.machineFxBorrowed());
  h.advance(250,250);assert(h.machineFxBorrowed());edge(h,key,false,251);assert(h.machineFxBorrowed()&&h.navMode()==7);
  assert(continuousTarget(h.routeForControl(40)).cell==1552+24*source+3);
  const uint8_t move[]{0xBE,4,64};auto out=h.receive(0,move,3,260,260);assert(out.emit&&out.api==718+24*source);h.sent(out,0);
  assert(h.desiredLive(1552+24*source+3).value==64&&!h.desiredLive(320+42*(machine-1)).known);
  edge(h,key,true,300);h.advance(500,500);assert(!h.machineFxBorrowed());edge(h,key,false,501);assert(!h.machineFxBorrowed());
  out=h.receive(0,move,3,510,510);assert(out.emit&&out.api==715+24*source);h.sent(out,0);
  edge(h,key,true,600);h.stop(650);h.advance(900,900);edge(h,key,false,901);assert(!h.machineFxBorrowed());
  edge(h,key,true,1000);h.advance(1200,1200);edge(h,key,false,1201);assert(h.machineFxBorrowed());
  bot(h,14,true,1300);bot(h,15,true,1310);h.advance(1510,1510);
  assert(h.navMode()==5&&h.machineFxBorrowed());
  const auto reset=h.routeForControl(20);assert(reset.address/1024==1);
  assert(continuousTarget({reset.address-1024,reset.position}).cell==1552+24*source+3);
  bot(h,15,false,1520);bot(h,14,false,1530);assert(h.navMode()==7&&h.machineFxBorrowed());
  const int next=machine==8?2:machine+2;const int nextKey=selector(n,next);edge(h,nextKey,true,1700);edge(h,nextKey,false,1701);
  assert(!h.machineFxBorrowed()&&continuousTarget(h.routeForControl(40)).cell==1552+24*((next-1)/2));
 }
 std::cout<<"PASS MP02/MP04/MP06 native/borrowed LONG once at200ms, realUP, SHORT entry native, STOP, canonical LIVE and unchanged MACHINE cells\n";
}
