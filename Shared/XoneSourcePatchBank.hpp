#pragma once
// Four contractual sources: INST1, INST2, YELLOW1, YELLOW2.
// PATCH owner 8..11 is a pair of MACHINE blocks, never a SAMPLE owner.
// S8 modes are configuration, outside all three PATCH payloads and BASE.
#include <array>
#include <cstdint>
namespace xone::host {
class SourcePatchBank {
public:
 static constexpr unsigned Sources=4,Slots=3,Fields=60;
 using Values=std::array<int,Fields>;
 struct Patch {bool valid{};int config{-2};std::uint32_t revision{};Values values{};Patch(){values.fill(-2);}};
 static int cell(unsigned source,unsigned field) noexcept {
  return source<Sources&&field<Fields?320+84*static_cast<int>(source)+42*static_cast<int>(field/30)+static_cast<int>(field%30):-1;
 }
 static int api(unsigned source,unsigned field) noexcept {
  if(source>=Sources||field>=Fields)return -1;
  int machine=2*static_cast<int>(source)+static_cast<int>(field/30),f=static_cast<int>(field%30);
  return f<18?128+18*machine+f:96+12*machine+f-18;
 }
 static int type(unsigned field) noexcept {return field%30<18?10:11;}
 static bool validate(const Values& values) noexcept {
  for(unsigned f=0;f<Fields;++f)if(values[f]<0||values[f]>127||(f%30>=18&&values[f]>1))return false;
  return true;
 }
 bool save(unsigned source,unsigned slot,int config,const Values& values) noexcept {
  if(source>=Sources||slot>=Slots||config<1||config>127||!validate(values))return false;
  auto& p=bank_[source*Slots+slot];if(p.revision>=16777212u||(p.valid&&p.config!=config))return false;
  p.valid=true;p.config=config;p.values=values;++p.revision;return true;
 }
 bool read(unsigned source,unsigned slot,Patch& p)const noexcept {
  if(source>=Sources||slot>=Slots)return false;
  p=bank_[source*Slots+slot];return p.valid;
 }
    // Explicit document fields, never object bytes/padding or output caches.
    template<class Archive> bool archive(Archive& a) noexcept {
        for(auto& p:bank_){a(p.valid);a(p.config);a(p.revision);a(p.values);
            if(p.valid){if(p.config<1||p.config>127||p.revision<1||p.revision>16777212u||!validate(p.values))return false;}
            else {if(p.revision!=0||p.config!=-2)return false;for(int v:p.values)if(v!=-2)return false;}
        }
        return a.ok();
    }
private:
 std::array<Patch,Sources*Slots> bank_{};
};
}
