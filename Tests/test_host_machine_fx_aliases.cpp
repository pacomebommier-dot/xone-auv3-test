#include "../CoreV3/MachineFxAliases.hpp"
#include "../Shared/XoneContinuousInput.hpp"
#include "../Core/LiveState.hpp"
#include <cassert>
#include <iostream>
using namespace xone::l01;
int main(){
 NavS3 initial;assert(initial.initializeDefault());
 for(int machine=1;machine<=8;++machine){auto n=initial.active();n.at(292)=7;n.contextMode=7;n.at(314)=machine;assert(NavS3::regenerate(n));
  for(bool borrowed:{false,true}){
   for(int j=4;j<56;++j){const auto route=MachineFxAliases::route(n,j,borrowed);
    if(machine%2||!((j>=20&&j<32)||(j>=40&&j<52))){assert(route==n.route(j));continue;}
    const int source=(machine-1)/2,index=j<32?j-20:j-40,column=index%4,row=index/4;
    auto fx=initial.active();const int root=source/2,page=1+4*(source%2)+column;
    fx.at(291)=root;if(page<5){fx.at(293+2*root)=page;fx.at(299+2*root)=1;}
    else {fx.at(294+2*root)=page;fx.at(300+2*root)=1;}
    assert(NavS3::regenerate(fx)&&NavS3::validate(fx));
    assert(route==fx.route((j<32?20:40)+4*row+(borrowed?1:0)));
    if(j>=40){const auto target=xone::host::continuousTarget(route);assert(target.valid());
     const int cell=1552+24*source+6*column+row+(borrowed?3:0);assert(target.cell==cell&&target.api==cell-837);
    }else if(!borrowed){int state=-1,mode=-1;assert(route.address/1024==3&&LiveState::spotAddresses(route.address,route.position,state,mode));
     assert(state==1768+12*source+3*column+row);
    }else {assert(route.address/1024==1);const auto target=xone::host::continuousTarget({route.address-1024,route.position});
     assert(target.valid()&&target.cell==1552+24*source+6*column+3+row);}
   }
  }
 }
 std::cout<<"PASS MP01/MP03 canonical aliases: four pairs, 12 CC/12 SPOT, native FX and borrowed RESET, odd pages and POT/FADER unchanged\n";
}
