#pragma once
// Native musical snapshots only. No gestures, pending requests, sent caches,
// backend acknowledgements or user-bank contents belong to this image.
#include "../Core/LiveState.hpp"
#include "../Core/NavS3.hpp"
#include "XoneGlobalMute.hpp"
#include "XoneSoftMidiBridge.hpp"
namespace xone::host {
struct GlobalSnapshot {
 static constexpr int Schema=1;
 int schema{Schema};bool valid{},muteVisible{};
 l01::LiveState::SessionImage live{};
 l01::NavImage nav{};
 GlobalMuteState::Applied mute{};
 SoftMidiBridge::Applied soft{};
 std::array<int,4> cueSelections{{-1,-1,-1,-1}};
 std::array<bool,96> memoryEngaged{};
 std::array<bool,4> presetEngaged{};
 static bool inScope(unsigned cell) noexcept {
  if(cell<320)return cell%40<28;
  if(cell<1328)return (cell-320)%42<30;
  return cell<1894&&cell!=1340;
 }
 static bool isState(unsigned cell) noexcept {
  if(cell<320)return cell%40>=16&&cell%40<28;
  if(cell<1328)return (cell-320)%42>=18&&(cell-320)%42<30;
  return (cell>=1477&&cell<1513)||(cell>=1768&&cell<1876);
 }
 bool admissible(int config)const noexcept {
  if(!valid||schema!=Schema||live.schema!=1||live.config!=config||!l01::NavS3::validate(nav))return false;
  for(unsigned cell=0;cell<live.desired.size();++cell){
   const int value=live.desired[cell];
   if(value!=-2&&(value<0||value>127||(isState(cell)&&value>1)))return false;
   if(!inScope(cell)&&value!=-2)return false;
  }
  for(unsigned group=0;group<4;++group){const int choice=cueSelections[group];
   if(choice!=-1&&(choice<0||choice>7||((group&1)==0?choice>3:choice<4)))return false;}
  if(mute.solo<-1||mute.solo>=15||((mute.solo>=0)!=mute.hasSnapshot))return false;
  for(unsigned i=0;i<16;++i)if(soft.choices[i]<-1||soft.choices[i]>3||(soft.choices[i]<0&&soft.sounding[i]))return false;
  return true;
 }
};
}
