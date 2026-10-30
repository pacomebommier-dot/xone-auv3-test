#pragma once
// Explicit numeric defaults for the neutral CC API, never inferred from boot.
// Configuration is metadata only: accepting it performs no musical reset.
#include <array>
#include <cstdint>
#include <cstddef>
#include "XoneSession.hpp"
namespace xone::host {
struct ResetDefaults {
 static constexpr unsigned Count=931;
 int config{};
 std::array<int,4> alternatives{{-1,-1,-1,-1}}; // expected native IDs; -1 = no alternative bound
 std::array<std::int16_t,Count> values{};
 ResetDefaults() noexcept {values.fill(-2);}
 static bool target(unsigned api)noexcept{return api<Count&&(api<569||api>573);}
 bool set(unsigned api,int value)noexcept {
  if(!target(api)||(value!=-2&&(value<0||value>127)))return false;
  values[api]=static_cast<std::int16_t>(value);return true;
 }
 bool valid()const noexcept {
  if(config<0||config>127)return false;
  for(int id:alternatives)if(id < -1 || id > 3 || (config==0&&id!=-1))return false;
  for(unsigned api=0;api<Count;++api){
   const int v=values[api];
   if(v!=-2&&(!target(api)||config==0||v<0||v>127))return false;
  }
  return true;
 }
 int get(int api,int configuration)const noexcept {
  return config==configuration&&api>=0&&target(static_cast<unsigned>(api))?values[static_cast<unsigned>(api)]:-2;
 }
};
using ResetDefaultsBytes=std::array<uint8_t,16+ResetDefaults::Count+4>;
inline bool encodeResetDefaults(const ResetDefaults& table,ResetDefaultsBytes& out)noexcept {
 if(!table.valid())return false;
 ResetDefaultsBytes bytes{};bytes[0]='X';bytes[1]='R';bytes[2]='S';bytes[3]='T';bytes[4]=1;
 sessionPut32(bytes.data()+8,static_cast<uint32_t>(table.config));
 for(unsigned s=0;s<4;++s)bytes[12+s]=static_cast<uint8_t>(table.alternatives[s]+2);
 for(unsigned i=0;i<ResetDefaults::Count;++i)bytes[16+i]=static_cast<uint8_t>(table.values[i]+2);
 sessionPut32(bytes.data()+bytes.size()-4,sessionCrc(bytes.data(),bytes.size()-4));out=bytes;return true;
}
inline bool decodeResetDefaults(const uint8_t* bytes,std::size_t size,ResetDefaults& out)noexcept {
 if(!bytes||size!=ResetDefaultsBytes{}.size()||bytes[0]!='X'||bytes[1]!='R'||bytes[2]!='S'||
    bytes[3]!='T'||bytes[4]!=1||bytes[5]||bytes[6]||bytes[7]||
    sessionGet32(bytes+8)>127||sessionGet32(bytes+size-4)!=sessionCrc(bytes,size-4))return false;
 ResetDefaults table;table.config=static_cast<int>(sessionGet32(bytes+8));
 for(unsigned s=0;s<4;++s){if(bytes[12+s]<1||bytes[12+s]>5)return false;table.alternatives[s]=static_cast<int>(bytes[12+s])-2;}
 for(unsigned i=0;i<ResetDefaults::Count;++i){
  const unsigned v=bytes[16+i];if(v==1||v>129)return false;
  table.values[i]=static_cast<int16_t>(static_cast<int>(v)-2);
 }
 if(!table.valid())return false;
 out=table;return true;
}

}
