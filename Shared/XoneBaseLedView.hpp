#pragma once
// Frozen L10 S7 @LedContext/@x4, BASE selector routes only.
// Unsupported context owners remain UNKNOWN, never fabricated as an OFF state.
#include "XoneMuteLedView.hpp"
#include "../Core/NavS3.hpp"
namespace xone::host {
struct BaseLed {MuteLedColor color{MuteLedColor::Off};bool blinking{};bool known{};};
class BaseLedView {
public:
 using Image=std::array<BaseLed,16>;
 static Image compose(const l01::NavS3& nav) noexcept {
  Image image{};if(!nav.stable())return image;
  const auto& n=nav.active();
  for(unsigned i=0;i<16;++i){
   const auto r=n.route(static_cast<int>(i+4));
   if(r.address==-1&&r.position==-1){image[i].known=true;continue;}
   if(r.address<0||r.position<0||r.address/1024!=2)continue;
   const int group=(r.address%1024)/32,value=r.address%32;
   auto& led=image[i];led.known=true;
   if(group==1){if(value==n.at(313))led.color=MuteLedColor::Yellow;}
   else if(group==2){
    if(value==n.at(314))led.color=value>8&&value<17?MuteLedColor::Green:
      (value>4&&value<9?MuteLedColor::Yellow:MuteLedColor::Red);
   }else if(group>=9&&group<=12){
    const int selection=296+group,rank=300+group;
    bool active=n.at(selection)==value&&n.at(rank)>0;
    for(int other=309;other<313;++other)
     if(other!=rank&&n.at(other)>0&&n.at(other)<n.at(rank)&&
        (other==312||rank==312||(other<311&&rank<311)))active=false;
    if(active)led.color=static_cast<MuteLedColor>(1+(group-9)%3);
   }else if(group==3||group==4){
    int selection=293+2*(group-3),flag=299+2*(group-3);
    if(value>4){++selection;++flag;}
    if(n.at(selection)==value&&n.at(flag)==1)led.color=static_cast<MuteLedColor>(group-2);
   }else if(group==5||group==6){
    if(n.at(292+group)==value&&n.at(298+group)==1)
     led.color=group==5?MuteLedColor::Green:MuteLedColor::Red;
   }
  }
  return image;
 }
};
}