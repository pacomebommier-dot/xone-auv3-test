#pragma once
// SH-only observation model. A configured table is not proof that a physical
// Roland sends those 36 CC on every recall. No MIDI is emitted by this class.
#include "XoneSession.hpp"
#include <array>
#include <cstdint>
#include <cstddef>
#include <limits>
namespace xone::host {
struct Sh01aProfile {
 uint32_t destination{};uint8_t channel{},cable{};uint32_t windowMs{};
 std::array<uint8_t,36> cc{};
 bool valid()const noexcept {
  if(!destination||channel>15||cable>15||!windowMs||windowMs>60000||(cable==0&&channel==14))return false;
  std::array<bool,128> seen{};
  for(auto n:cc){if(n>127||seen[n])return false;seen[n]=true;}return true;
 }
 bool operator==(const Sh01aProfile& b)const noexcept{return destination==b.destination&&channel==b.channel&&cable==b.cable&&windowMs==b.windowMs&&cc==b.cc;}
};
struct Sh01aPatchMirror {
 bool valid{};uint32_t revision{};std::array<int16_t,36> values{};
 Sh01aPatchMirror()noexcept{values.fill(-2);}
 bool admissible()const noexcept {
  if(valid!=(revision!=0))return false;
  for(auto v:values)if(valid?(v<0||v>127):v!=-2)return false;
  return true;
 }
 bool operator==(const Sh01aPatchMirror& b)const noexcept{return valid==b.valid&&revision==b.revision&&values==b.values;}
};
struct Sh01aMirrorImage {
 Sh01aProfile profile{};std::array<Sh01aPatchMirror,3> mirrors{};
 bool valid()const noexcept{if(!profile.valid())return false;for(const auto& m:mirrors)if(!m.admissible())return false;return true;}
 bool operator==(const Sh01aMirrorImage& b)const noexcept{return profile==b.profile&&mirrors==b.mirrors;}
};
using Sh01aMirrorBytes=std::array<uint8_t,18+36+3*(5+36)+4>;
inline bool encodeSh01aMirror(const Sh01aMirrorImage& image,Sh01aMirrorBytes& out)noexcept {
 if(!image.valid())return false;
 Sh01aMirrorBytes b{};b[0]='S';b[1]='H';b[2]='3';b[3]='6';b[4]=1;
 sessionPut32(b.data()+8,image.profile.destination);b[12]=image.profile.channel;b[13]=image.profile.cable;
 sessionPut32(b.data()+14,image.profile.windowMs);unsigned at=18;
 for(auto n:image.profile.cc)b[at++]=n;
 for(const auto& m:image.mirrors){b[at++]=m.valid?1:0;sessionPut32(b.data()+at,m.revision);at+=4;for(auto v:m.values)b[at++]=static_cast<uint8_t>(v+2);}
 sessionPut32(b.data()+at,sessionCrc(b.data(),at));out=b;return true;
}
inline bool decodeSh01aMirror(const uint8_t* b,std::size_t size,Sh01aMirrorImage& out)noexcept {
 if(!b||size!=Sh01aMirrorBytes{}.size()||b[0]!='S'||b[1]!='H'||b[2]!='3'||b[3]!='6'||b[4]!=1||b[5]||b[6]||b[7]||sessionGet32(b+size-4)!=sessionCrc(b,size-4))return false;
 Sh01aMirrorImage image;image.profile.destination=sessionGet32(b+8);image.profile.channel=b[12];image.profile.cable=b[13];image.profile.windowMs=sessionGet32(b+14);unsigned at=18;
 for(auto& n:image.profile.cc)n=b[at++];
 for(auto& m:image.mirrors){if(b[at]>1)return false;m.valid=b[at++]!=0;m.revision=sessionGet32(b+at);at+=4;for(auto& v:m.values){const unsigned raw=b[at++];if(raw==1||raw>129)return false;v=static_cast<int16_t>(static_cast<int>(raw)-2);}}
 if(!image.valid())return false;
 out=image;return true;
}
class Sh01aMirror {
public:
 struct Observation {bool accepted{},mirrorReplaced{};int index{-1},value{-2};};
 Sh01aMirror()noexcept{live_.fill(-2);}
 bool configure(const Sh01aProfile& profile)noexcept {
  if(!profile.valid())return false;
  // A destination change must never relabel another instrument's three mirrors.
  if(configured_&&!(image_.profile==profile))return false;
  image_.profile=profile;configured_=true;return true;
 }
 bool restore(const Sh01aMirrorImage& image)noexcept {
  if(!image.valid())return false;
  image_=image;configured_=true;live_.fill(-2);program_=-1;programObserved_=false;capture_=false;count_=0;lastTime_=-1;incomplete_.fill(false);incompleteReport_={};extra_={};return true;
 }
 const Sh01aMirrorImage& snapshot()const noexcept{return image_;}
 int program()const noexcept{return program_;}
 bool programObserved()const noexcept{return programObserved_;}
 bool liveKnown()const noexcept{for(auto v:live_)if(v<0)return false;return true;}
 struct CaptureReport {
  int program{-1};
  unsigned count{};
  std::array<uint64_t,2> missing{},extra{};
 };
 CaptureReport takeIncomplete() noexcept {
  const auto report=incompleteReport_;
  incompleteReport_={};
  return report;
 }
 bool capturing()const noexcept{return capture_;}
 unsigned received()const noexcept{return count_;}
 bool incomplete(unsigned slot)const noexcept{return slot<3&&incomplete_[slot];}
 int live(unsigned cc)const noexcept{const int i=index(cc);return i<0?-2:live_[static_cast<unsigned>(i)];}
 bool altered(unsigned slot)const noexcept {
  if(slot>=3||program_!=static_cast<int>(slot)||!image_.mirrors[slot].valid)return false;
  for(auto v:live_)if(v<0)return false;
  return live_!=image_.mirrors[slot].values;
 }
 void advance(int64_t now)noexcept {
  if(now<0||now<lastTime_)return;
  if(capture_&&now-started_>static_cast<int64_t>(image_.profile.windowMs))cancel();
  lastTime_=now;
 }
 void stop()noexcept{cancel();}
 bool localEdit(unsigned cc,int value)noexcept {
  const int i=index(cc);if(i<0||value<0||value>127)return false;
  cancel();live_[static_cast<unsigned>(i)]=static_cast<int16_t>(value);return true;
 }
 // Called only after successful output callback of the corresponding PC.
 // Success means transport emission, never physical WRITE or instrument ACK.
 bool recallSent(unsigned program,int64_t now)noexcept {
  if(!configured_||program>63||now<0||now<lastTime_)return false;
  start(program,now);programObserved_=false;return true;
 }
 Observation receive(uint8_t cable,const uint8_t* bytes,std::size_t size,int64_t now)noexcept {
  if(!configured_||!bytes||now<0||now<lastTime_||cable!=image_.profile.cable||!size||(bytes[0]&15)!=image_.profile.channel)return {};
  const unsigned command=bytes[0]&0xF0;
  if(command==0xC0&&size==2&&bytes[1]<64){start(bytes[1],now);programObserved_=true;return {true,false,-1,-2};}
  if(command!=0xB0||size!=3||bytes[1]>127||bytes[2]>127)return {};
  const int i=index(bytes[1]);
  advance(now);
  if(i<0){
   if(capture_)mark(extra_,bytes[1]);
   return {};
  }
  const auto at=static_cast<unsigned>(i);
  live_[at]=bytes[2];bool replaced=false;
  if(capture_){
   staging_[at]=bytes[2];if(!seen_[at]){seen_[at]=true;++count_;}
   if(count_==36){
    auto& m=image_.mirrors[static_cast<unsigned>(program_)];
    if(m.revision!=std::numeric_limits<uint32_t>::max()){
     m.values=staging_;++m.revision;m.valid=true;programObserved_=true;incomplete_[static_cast<unsigned>(program_)]=false;replaced=true;
    }else reportIncomplete();
    capture_=false;
   }
  }
  return {true,replaced,i,bytes[2]};
 }
private:
 int index(unsigned cc)const noexcept {
  if(!configured_||cc>127)return -1;
  for(unsigned i=0;i<36;++i)if(image_.profile.cc[i]==cc)return static_cast<int>(i);
  return -1;
 }
 static void mark(std::array<uint64_t,2>& mask,unsigned cc)noexcept {
  mask[cc/64]|=uint64_t{1}<<(cc%64);
 }
 void reportIncomplete()noexcept {
  incomplete_[static_cast<unsigned>(program_)]=true;
  incompleteReport_={program_,count_,{},extra_};
  for(unsigned i=0;i<36;++i)if(!seen_[i])mark(incompleteReport_.missing,image_.profile.cc[i]);
 }
 void cancel()noexcept {
  if(capture_)reportIncomplete();
  capture_=false;count_=0;seen_.fill(false);
 }
 void start(unsigned program,int64_t now)noexcept {
  cancel();program_=static_cast<int>(program);lastTime_=now;started_=now;
  capture_=program<3;extra_={};seen_.fill(false);staging_.fill(-2);live_.fill(-2);
 }
 CaptureReport incompleteReport_{};
 std::array<uint64_t,2> extra_{};
 Sh01aMirrorImage image_{};bool configured_{},capture_{},programObserved_{};int program_{-1};unsigned count_{};
 int64_t lastTime_{-1},started_{};std::array<int16_t,36> live_{},staging_{};
 std::array<bool,36> seen_{};std::array<bool,3> incomplete_{};
};
}
