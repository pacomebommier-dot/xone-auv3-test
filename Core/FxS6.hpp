#pragma once
// L01/10 diagnostic port of S6 @FxObserve/@FxOwnerStatus/@FxSnapshot.
// No SysEx routing or MIDI I/O, no persistent-bank writer, no fabricated MATCH.
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::l01 {
struct FxEvent {int owner{-1},field{-1},value{-2},config{};bool known{};std::uint32_t epoch{};};
struct FxStatus {int owner{-1},slot{-1},index{-1},valid{-2},equal{-2},config{};std::uint32_t epoch{};};
class FxS6 {
public:
 static constexpr int Unknown=-2;
 static constexpr std::size_t OwnerCount=28,FieldCount=9,SlotCount=96,QueueMax=384;
 static constexpr std::uint32_t EpochMax=16777213;
 struct MemorySnapshot {bool supplied{},valid{};int config{};std::uint32_t revision{};std::array<int,FieldCount> values{};};
 FxS6() noexcept {live_.fill(Unknown);sent_.fill(-1);sentEpoch_.fill(0);}
 static int slotIndex(int owner,int slot)noexcept {
  if(owner<0||owner>=28||slot<0)return -1;
  if(owner<12)return slot<4?4*owner+slot:-1;
  return slot<3?48+3*(owner-12)+slot:-1;
 }
 static int logicalIndex(int owner)noexcept {
  if(owner<0||owner>27)return -1;
  if(owner>=12)return owner+4;
  const int group=owner/6;const int local=owner%6;
  return group*8+local+(local>=3?1:0);
 }
 static int addressFor(int owner,int field)noexcept {
  const int logical=logicalIndex(owner);
  if(logical<0||field<0||field>8)return -1;
  return field<6?1552+6*logical+field:1768+3*logical+(field-6);
 }
 static bool decodeAddress(int address,int& owner,int& field)noexcept {
  owner=-1;field=-1;int logical=-1;
  if(address>=1552&&address<=1743){logical=(address-1552)/6;field=(address-1552)%6;}
  else if(address>=1768&&address<=1863){logical=(address-1768)/3;field=6+(address-1768)%3;}
  else return false;
  if(logical<16){const int k=logical%8;
   if(k==3||k==7)return false;
   owner=6*(logical/8)+k-(k>=4?1:0);
  }else owner=logical-4;
  return owner>=0&&owner<28&&field>=0&&field<9&&addressFor(owner,field)==address;
 }
 // The caller must check host-origin token separately; this is a strict
 // adapter for a retained S6 20-byte source fixture, not a native wire protocol.
 static bool decodeFrame(const std::array<int,20>& r,FxEvent& event)noexcept {
  constexpr int header[9]={125,88,75,50,7,13,4,6,20};
  for(int k=0;k<20;++k)if(r[k]<0||r[k]>127)return false;
  for(int k=0;k<9;++k)if(r[k]!=header[k])return false;
  if(r[11]>1||r[14]>1||r[13]!=127||
     (r[11]==0&&r[12]!=0)||(r[14]==0&&r[15]!=0))return false;
  const std::uint32_t e=static_cast<std::uint32_t>(r[16])+128u*static_cast<std::uint32_t>(r[17])+
    16384u*static_cast<std::uint32_t>(r[18])+2097152u*static_cast<std::uint32_t>(r[19]);
  if(e<1||e>EpochMax)return false;
  int owner=-1,field=-1;
  if(!decodeAddress(r[9]+128*r[10],owner,field))return false;
  if(field>=6&&r[11]==1&&r[12]>1)return false;
  event={owner,field,r[11]==1?r[12]:Unknown,r[14]==1?r[15]:0,r[11]==1,e};return true;
 }
 static bool isSnapshot(const std::array<int,8>& r)noexcept {
  constexpr int header[8]={125,88,75,50,7,17,7,6};
  for(int k=0;k<8;++k)if(r[k]!=header[k])return false;
  return true;
 }
 // Snapshots are only staged as explicit trusted diagnostic data.
 bool supplyMemory(int owner,int slot,const MemorySnapshot& m)noexcept {
  const int index=slotIndex(owner,slot);
  if(index<0||!m.supplied)return false;
  if(m.valid){if(m.config<1||m.config>127||m.revision<1||m.revision>EpochMax)return false;
   for(int k=0;k<9;++k){const int v=m.values[static_cast<std::size_t>(k)];
    if(v<0||v>127||(k>=6&&v>1))return false;}
  }else if(m.config!=0||m.revision!=0)return false;
  memory_[static_cast<std::size_t>(index)]=m;return true;
 }
 // A native bank save may refresh the affected FX memory status immediately.
 // All writes/queue growth must be committed together by the coordinator.
 bool publishNativeMemory(int owner,int slot,const MemorySnapshot& m)noexcept {
  const int index=slotIndex(owner,slot);
  if(index<0||!m.supplied||count_+1>QueueMax)return false;
  if(!supplyMemory(owner,slot,m))return false;
  if(config_!=0)publish(owner,slot,true);
  return true;
 }
 bool accept(const FxEvent& e)noexcept {
  if(e.owner<0||e.owner>=28||e.field<0||e.field>=9||e.epoch==0||e.epoch>EpochMax||e.epoch<epoch_||
     e.config<0||e.config>127||(e.known&&(e.value<0||e.value>127||(e.field>=6&&e.value>1)))||
     (!e.known&&e.value!=Unknown))return false;
  const bool all=(e.config!=config_);
  const std::size_t need=all?SlotCount:static_cast<std::size_t>(e.owner<12?4:3);
  if(count_+need>QueueMax)return false; // transactional and bounded
  if(all)live_.fill(Unknown); // avoid cross-profile false MATCH from old FX observations
  config_=e.config;epoch_=e.epoch;live_[9*static_cast<std::size_t>(e.owner)+static_cast<std::size_t>(e.field)]=e.value;
  if(all){for(int o=0;o<28;++o)for(int s=0;s<(o<12?4:3);++s)publish(o,s);}
  else for(int s=0;s<(e.owner<12?4:3);++s)publish(e.owner,s);
  return true;
 }
 bool snapshot()noexcept {
  if(count_+SlotCount>QueueMax)return false;
  // Snapshot is an explicit refresh; never writes banks or LIVE.
  for(int o=0;o<28;++o)for(int s=0;s<(o<12?4:3);++s)publish(o,s,true);
  return true;
 }
 int field(int owner,int idx)const noexcept {if(owner<0||owner>=28||idx<0||idx>=9)return Unknown;return live_[9*static_cast<std::size_t>(owner)+static_cast<std::size_t>(idx)];}
 int config()const noexcept{return config_;}
 std::uint32_t epoch()const noexcept{return epoch_;}
 std::size_t pending()const noexcept{return count_;}
 FxStatus status(std::size_t i)const noexcept{return i<count_?queue_[i]:FxStatus{};}
 void drain()noexcept{count_=0;}
 void clearObserved()noexcept {live_.fill(Unknown);config_=0;epoch_=0;sent_.fill(-1);sentEpoch_.fill(0);count_=0;}
private:
 FxStatus makeStatus(int owner,int slot)const noexcept {
  const int index=slotIndex(owner,slot);
  FxStatus st{owner,slot,index,Unknown,Unknown,config_,epoch_};
  const auto& m=memory_[static_cast<std::size_t>(index)];
  if(!m.supplied||config_==0)return st;
  if(!m.valid){st.valid=0;return st;}
  if(m.config!=config_)return st;
  st.valid=1;st.equal=1;
  for(int i=0;i<9;++i){const int actual=field(owner,i);
   if(actual<0){st.equal=Unknown;return st;}
   if(actual!=m.values[static_cast<std::size_t>(i)])st.equal=0;
  }
  return st;
 }
 void publish(int owner,int slot,bool force=false)noexcept {
  const FxStatus st=makeStatus(owner,slot);const auto index=static_cast<std::size_t>(st.index);
  const int composite=st.valid+2+4*(st.equal+2)+16*st.config;
  if(force||sent_[index]!=composite||sentEpoch_[index]!=st.epoch){
   sent_[index]=composite;sentEpoch_[index]=st.epoch;queue_[count_++]=st;
  }
 }
 std::array<int,OwnerCount*FieldCount> live_{};
 std::array<MemorySnapshot,SlotCount> memory_{};
 std::array<int,SlotCount> sent_{};
 std::array<std::uint32_t,SlotCount> sentEpoch_{};
 std::array<FxStatus,QueueMax> queue_{};
 std::size_t count_{};int config_{};std::uint32_t epoch_{};
};
} // namespace xone::l01
