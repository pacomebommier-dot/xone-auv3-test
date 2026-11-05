#pragma once
// Versioned host document: explicit scalar fields, CRC, bounded decoder.
// No C++ object representation, pending command, physical gesture or sent cache.
#include "XoneSession.hpp"
#include "XoneGlobalSnapshot.hpp"
#include "XoneS8SamplePatchBank.hpp"
#include "XoneS8MachinePatchBank.hpp"
#include "XoneSourcePatchBank.hpp"
#include "XoneAlternativeBank.hpp"
#include "../Core/BankS6.hpp"
#include <algorithm>
#include <vector>
#include <limits>
#include <type_traits>
namespace xone::host {
// BASE uses canonical MACHINE/FX cells. Slots 0..3 belong to unbound sources,
 // and slots 4..11 follow the two pools of instrument alternatives.
struct SourceBaseImage {
 static constexpr unsigned Count=73;
 bool valid{};
 int config{};
 std::array<int,Count> values{};
};
struct ShSourceBaseImage {
 uint32_t destination{};
 int source{-1},alternative{-1};
 SourceBaseImage base{};
};
struct DocumentImage {
 SessionImage live{};
 GlobalSnapshot current{};
 S8SamplePatchBank sample{};
 S8MachinePatchBank machine{};
 SourcePatchBank source{};
 AlternativeBank alternative{};
 std::array<SourceBaseImage,12> sourceBases{};
 ShSourceBaseImage shBase{};
 std::array<l01::BankS6::MemoryRecord,96> memories{};
 std::array<l01::BankS6::PatchRecord,84> legacyPatches{};
 std::array<GlobalSnapshot,4> presets{};
 GlobalSnapshot init{};
};
struct DocumentValidator {template<class T> void operator()(T&)const noexcept {} bool ok()const noexcept{return true;}};
using DocumentBytes=std::vector<uint8_t>;
constexpr std::size_t MaxDocumentBytes=131072;
class DocumentWire {
public:
 explicit DocumentWire(DocumentBytes& out):out_(&out){}
 DocumentWire(const uint8_t* data,std::size_t size):data_(data),size_(size){}
 template<class T,std::size_t N> void operator()(std::array<T,N>& values)noexcept {for(auto& v:values)(*this)(v);}
 template<class T> void operator()(T& value)noexcept {
  static_assert(std::is_integral_v<T>);
  if(!good_)return;
  if(out_){
   if(out_->size()+4>MaxDocumentBytes){good_=false;return;}
   const auto n=static_cast<uint32_t>(static_cast<int64_t>(value));
   for(unsigned i=0;i<4;++i)out_->push_back(static_cast<uint8_t>(n>>(8*i)));
  }else{
   if(at_+4>size_){good_=false;return;}
   const uint32_t raw=sessionGet32(data_+at_);at_+=4;
   const int64_t n=std::is_unsigned_v<T>?static_cast<int64_t>(raw):(raw<=0x7FFFFFFFu?static_cast<int64_t>(raw):static_cast<int64_t>(raw)-4294967296LL);
   if(n<static_cast<int64_t>(std::numeric_limits<T>::lowest())||n>static_cast<int64_t>(std::numeric_limits<T>::max())){good_=false;return;}
   value=static_cast<T>(n);
  }
 }
 bool ok()const noexcept{return good_;}
 bool complete()const noexcept{return good_&&(out_||at_==size_);}
private:
 DocumentBytes* out_{};const uint8_t* data_{};std::size_t size_{},at_{};bool good_{true};
};
template<class A> inline void documentLive(A& a,SessionImage& s)noexcept {a(s.schema);a(s.config);a(s.revision);a(s.desired);}
template<class A> inline bool documentSnapshot(A& a,GlobalSnapshot& g,int config)noexcept {
 a(g.valid);if(!g.valid){g=GlobalSnapshot{};return a.ok();}
 a(g.schema);a(g.muteVisible);documentLive(a,g.live);a(g.nav.cells);a(g.nav.contextMode);a(g.nav.presetFlag);a(g.nav.sampleSlot);
 a(g.mute.layer);a(g.mute.beforeSolo);a(g.mute.solo);a(g.mute.hasSnapshot);
 a(g.soft.choices);a(g.soft.sounding);a(g.alternatives);a(g.cueSelections);a(g.memoryEngaged);a(g.presetEngaged);
 return a.ok()&&validSession(g.live)&&g.admissible(config);
}
template<class A> inline bool documentFields(A& a,DocumentImage& d)noexcept {
 documentLive(a,d.live);if(!a.ok()||!validSession(d.live)||!documentSnapshot(a,d.current,d.live.config)||!d.current.valid)return false;
 for(unsigned c=0;c<d.live.desired.size();++c)if(GlobalSnapshot::inScope(c)&&d.current.live.desired[c]!=d.live.desired[c])return false;
 for(unsigned c=0;c<1328;++c){
  const bool mode=c<320?c%40>=28:(c-320)%42>=30;
  const int value=d.live.desired[c];if(mode&&value!=-2&&(value<1||value>4))return false;
 }
 if(!d.sample.archive(a)||!d.machine.archive(a)||!d.source.archive(a)||!d.alternative.archive(a))return false;
 for(auto& m:d.memories){a(m.valid);a(m.config);a(m.revision);a(m.values);
  if(m.valid){if(m.config!=d.live.config||m.revision<1||m.revision>=l01::BankS6::MaxRevision||!l01::BankS6::validateMemory(m.values))return false;}
  else {if(m.config||m.revision)return false;for(int v:m.values)if(v!=-2)return false;}
 }
 for(unsigned i=0;i<d.legacyPatches.size();++i){auto& p=d.legacyPatches[i];a(p.valid);a(p.config);a(p.revision);a(p.values);
  if(p.valid){if(p.config!=d.live.config||p.revision<1||p.revision>=l01::BankS6::MaxRevision||!l01::BankS6::validatePatch(static_cast<int>(i/3),p.values))return false;}
  else {if(p.config||p.revision)return false;for(int v:p.values)if(v!=-2)return false;}
 }
 for(auto& p:d.presets)if(!documentSnapshot(a,p,d.live.config))return false;
 if(!documentSnapshot(a,d.init,d.live.config))return false;
 for(unsigned i=0;i<96;++i)if(d.current.memoryEngaged[i]&&!d.memories[i].valid)return false;
 for(unsigned i=0;i<4;++i)if(d.current.presetEngaged[i]&&!d.presets[i].valid)return false;
 // Active IDs must have an explicit configured local BASE. No synthetic binding.
 for(unsigned s=0;s<4;++s){SourcePatchBank::Values base{};const int id=d.current.alternatives[s];
  if(id>=0&&!d.alternative.base(s/2,static_cast<unsigned>(id),d.live.config,base))return false;
 }
 return a.ok();
}
template<class Archive> inline bool documentSourceBase(Archive& archive,SourceBaseImage& base) noexcept {
 archive(base.valid);archive(base.config);archive(base.values);
 if(base.valid?(base.config<1||base.config>127):base.config!=0)return false;
 for(unsigned i=0;i<base.values.size();++i){
  const int maxValue=(i>=18&&i<30)||(i>=60&&i<72)?1:127;
  if(base.values[i]<0||base.values[i]>maxValue||(!base.valid&&base.values[i]!=0))return false;
 }
 return archive.ok();
}
template<class Archive> inline bool documentSourceBases(Archive& archive,DocumentImage& document) noexcept {
 for(auto& base:document.sourceBases)if(!documentSourceBase(archive,base))return false;
 return archive.ok();
}
template<class Archive> inline bool documentShBase(Archive& archive,ShSourceBaseImage& image) noexcept {
 archive(image.destination);archive(image.source);archive(image.alternative);
 if(!documentSourceBase(archive,image.base))return false;
 if(!image.base.valid)return image.destination==0&&image.source==-1&&image.alternative==-1;
 if(!image.destination||image.source<0||image.source>3||image.alternative<-1||image.alternative>3)return false;
 for(unsigned f=0;f<36;++f)if(image.base.values[f])return false;
 return true;
}
inline bool validDocument(DocumentImage& d)noexcept {
 DocumentValidator a;return documentFields(a,d)&&documentSourceBases(a,d)&&documentShBase(a,d.shBase);
}
inline bool encodeDocument(const DocumentImage& image,DocumentBytes& out) {
 auto d=image;const bool extended=std::any_of(d.sourceBases.begin(),d.sourceBases.end(),[](const auto& base){return base.valid;});
 DocumentBytes bytes{'X','D','O','C',static_cast<uint8_t>(d.shBase.base.valid?4:extended?3:2),0,0,0};bytes.reserve(MaxDocumentBytes);
 DocumentValidator validator;
 if(!documentSourceBases(validator,d)||!documentShBase(validator,d.shBase))return false;
 DocumentWire wire(bytes);if(!documentFields(wire,d)||(bytes[4]>=3&&!documentSourceBases(wire,d))||(bytes[4]==4&&!documentShBase(wire,d.shBase))||bytes.size()+4>MaxDocumentBytes)return false;
 const auto crc=sessionCrc(bytes.data(),bytes.size());for(unsigned i=0;i<4;++i)bytes.push_back(static_cast<uint8_t>(crc>>(8*i)));
 out=std::move(bytes);return true;
}
inline bool decodeDocument(const uint8_t* bytes,std::size_t size,DocumentImage& out)noexcept {
 if(!bytes||size<12||size>MaxDocumentBytes||bytes[0]!='X'||bytes[1]!='D'||bytes[2]!='O'||bytes[3]!='C'||(bytes[4]<2||bytes[4]>4)||bytes[5]||bytes[6]||bytes[7]||sessionGet32(bytes+size-4)!=sessionCrc(bytes,size-4))return false;
 DocumentImage d{};DocumentWire wire(bytes+8,size-12);
 if(!documentFields(wire,d)||(bytes[4]>=3&&!documentSourceBases(wire,d))||(bytes[4]==4&&!documentShBase(wire,d.shBase))||!wire.complete())return false;
 out=d;return true;
}
struct DocumentFrame {DocumentImage image{};uint32_t token{};};
struct DocumentExchange {
 StateMailbox<DocumentFrame> restores{},snapshots{};
 DocumentFrame ui{};uint32_t requested{},applied{};
};
}
