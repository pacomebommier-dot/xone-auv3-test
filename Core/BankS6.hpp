#pragma once
// L01/11: Native, volatile, validated PATCH (family 1) and MEMORY (family 2).
// Frozen S6 @MemResolve and @MemCheckTypedValue. Does not implement legacy CUE
// family 3, SysEx wire messages, hardware routing, or AUM persistence.
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::l01 {
class BankS6 {
public:
 static constexpr int Unknown=-2;
 static constexpr std::uint32_t MaxRevision=16777213;
 static constexpr int PatchOwners=28,PatchSlots=3,PatchRecords=84,MemoryRecords=96;
 using PatchImage=std::array<int,84>;
 using MemoryImage=std::array<int,9>;
 struct PatchRecord { bool valid{};int config{};std::uint32_t revision{};PatchImage values{};
  PatchRecord() noexcept {values.fill(Unknown);} };
 struct MemoryRecord { bool valid{};int config{};std::uint32_t revision{};MemoryImage values{};
  MemoryRecord() noexcept {values.fill(Unknown);} };
 static constexpr int patchWidth(int owner) noexcept {
  return owner<0||owner>=PatchOwners?0:(owner<8?40:(owner<12?84:42));
 }
 static constexpr int patchIndex(int owner,int slot) noexcept {
  return owner<0||owner>=PatchOwners||slot<0||slot>=PatchSlots?-1:3*owner+slot;
 }
 static constexpr int memoryIndex(int owner,int slot) noexcept {
  if(owner<0||owner>=28||slot<0)return -1;
  return owner<12?(slot<4?4*owner+slot:-1):(slot<3?48+3*(owner-12)+slot:-1);
 }
 static bool validatePatch(int owner,const PatchImage& v) noexcept {
  const int n=patchWidth(owner);if(n==0)return false;
  for(int i=0;i<84;++i){const int x=v[static_cast<std::size_t>(i)];
   if(i>=n){if(x!=Unknown)return false;continue;}
   if(x<0||x>127)return false;
   const int sub=owner<8?i:(i%42);
   if(owner<8){if(sub>=16&&sub<28&&x>1)return false;
     if(sub>=28&&(x<1||x>4))return false;
   }else{if(sub>=18&&sub<30&&x>1)return false;
     if(sub>=30&&(x<1||x>4))return false;}
  }return true;
 }
 static bool validateMemory(const MemoryImage& v) noexcept {
  for(int i=0;i<9;++i){int x=v[static_cast<std::size_t>(i)];
   if(x<0||x>127||(i>=6&&x>1))return false;
  }return true;
 }
 bool initializeEmpty() noexcept {
  if(initialized_)return false;
  // Explicit new empty native banks, not imported legacy snapshots.
  initialized_=true;generation_=1;return true;
 }
 bool initialized()const noexcept{return initialized_;}
 std::uint32_t generation()const noexcept{return generation_;}
 bool savePatch(int owner,int slot,int config,const PatchImage& image,
                std::uint32_t expectedRevision)noexcept {
  const int index=patchIndex(owner,slot);
  if(!initialized_||index<0||config<1||config>127||!validatePatch(owner,image)||generation_>=MaxRevision)return false;
  auto& p=patches_[static_cast<std::size_t>(index)];
  if(expectedRevision!=p.revision||p.revision>=MaxRevision-1 ||
     (p.valid&&p.config!=config))return false;
  p.values=image;p.valid=true;p.config=config;++p.revision;++generation_;return true;
 }
 bool saveMemory(int owner,int slot,int config,const MemoryImage& image,
                 std::uint32_t expectedRevision)noexcept {
  const int index=memoryIndex(owner,slot);
  if(!initialized_||index<0||config<1||config>127||!validateMemory(image)||generation_>=MaxRevision)return false;
  auto& m=memory_[static_cast<std::size_t>(index)];
  if(expectedRevision!=m.revision||m.revision>=MaxRevision-1 ||
     (m.valid&&m.config!=config))return false;
  m.values=image;m.valid=true;m.config=config;++m.revision;++generation_;return true;
 }
 // Requests have no read-side effects. Uninitialized/empty must remain invalid,
 // not a valid image consisting of zero or an inferred BASE/PATCH.
 bool readPatch(int owner,int slot,PatchRecord& out)const noexcept {
  const int idx=patchIndex(owner,slot);if(!initialized_||idx<0)return false;
  out=patches_[static_cast<std::size_t>(idx)];return true;
 }
 bool readMemory(int owner,int slot,MemoryRecord& out)const noexcept {
  const int idx=memoryIndex(owner,slot);if(!initialized_||idx<0)return false;
  out=memory_[static_cast<std::size_t>(idx)];return true;
 }
private:
 std::array<PatchRecord,PatchRecords> patches_{};
 std::array<MemoryRecord,MemoryRecords> memory_{};
 bool initialized_{};std::uint32_t generation_{};
};
} // namespace xone::l01
