#pragma once
// Canonical routes only: same FX cells, modes, owners and pickup as the
// existing INST/YELLOW pages. No LIVE values, FX engine or bank is stored.
#include "../Core/NavS3.hpp"
namespace xone::l01 {
class MachineFxAliases {
public:
 static NavRoute route(const NavImage& machine,int physical,bool borrowed)noexcept {
  const auto original=machine.route(physical);const int selected=machine.at(314);
  if(machine.contextMode!=7||
     selected<2||selected>8||selected%2||
     !((physical>=20&&physical<32)||(physical>=40&&physical<52)))return original;
  const int source=(selected-1)/2,root=source/2;
  const int index=physical<32?physical-20:physical-40,column=index%4,row=index/4;
  const int page=1+4*(source%2)+column;
  // Existing FX route identities: native SPOT kind3, secondary RESET kind1.
  // Compare against the frozen NAV mapper in MP01/MP03, without rebuilding
  // an entire NAV image each time a control or LED asks for its route.
  const int kind=physical<32?((borrowed||machine.at(292)==5)?1:3):0;
  return {kind*1024+32*(3+root)+page,row+(borrowed?3:0)};
 }
};
}
