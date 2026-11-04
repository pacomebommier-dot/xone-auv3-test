#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static void key(HostEngine& h,int physical,bool down,int ms){
 int note=-1;for(int n=24;n<40;++n)if(xone::l01::SurfaceInput::physicalControl(n)==physical)note=n;
 assert(note>=0);const uint8_t b[]{uint8_t(down?0x9E:0x8E),uint8_t(note),uint8_t(down?127:0)};assert(!h.receive(0,b,3,ms,ms).emit);
}
int main(){
 for(unsigned source=0;source<4;++source)for(int state:{-2,0,1}){
  xone::l01::NavS3 initial;assert(initial.initializeDefault());auto pair=initial.active();pair.at(292)=7;pair.contextMode=7;pair.at(314)=2*(source+1);
  assert(xone::l01::NavS3::regenerate(pair));SessionImage s{};s.schema=1;s.config=1;s.revision=1;s.desired.fill(-2);
  for(unsigned f=0;f<12;++f)s.desired[1768+12*source+f]=state;
  HostEngine alias;assert(alias.setNavForDiagnostic(pair)&&alias.restoreSessionBeforeRendering(s));
  for(bool borrowed:{false,true}){
   if(borrowed){int selector=-1;for(int p=4;p<20;++p)if(pair.route(p).address==2*1024+2*32+pair.at(314))selector=p;
    key(alias,selector,true,10);alias.advance(210,210);key(alias,selector,false,211);assert(alias.machineFxBorrowed());}
   auto actual=alias.ledImage();
   for(int column=0;column<4;++column){auto fx=initial.active();const int root=source/2,page=1+4*(source%2)+column;
    fx.at(291)=root;if(page<5){fx.at(293+2*root)=page;fx.at(299+2*root)=1;}else{fx.at(294+2*root)=page;fx.at(300+2*root)=1;}
    assert(xone::l01::NavS3::regenerate(fx));HostEngine native;assert(native.setNavForDiagnostic(fx)&&native.restoreSessionBeforeRendering(s));auto expected=native.ledImage();
    for(int row=0;row<3;++row){const auto a=actual[16+4*row+column],b=expected[16+4*row+(borrowed?1:0)];
     assert(a.known==b.known&&a.color==b.color&&a.blinking==b.blinking);}
   }
  }
 }
 std::cout<<"PASS MP03/MP-D native and borrowed LED identity, four pairs/four FX pages, UNKNOWN/OFF/ON\n";
}
