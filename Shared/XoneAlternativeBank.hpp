#pragma once
// Native logical instrument records. IDs are local to one of the two pools;
// they are not AUM strip numbers, channels, programs or external observations.
#include "XoneSourcePatchBank.hpp"
namespace xone::host {
class AlternativeBank {
public:
 using Values=SourcePatchBank::Values;
 using Patch=SourcePatchBank::Patch;
 AlternativeBank() noexcept {for(auto& row:bindings_)row.fill(-1);}
 bool bind(unsigned group,unsigned slot,int instrument) noexcept {
  if(group>=4||slot>=4||instrument<0||instrument>3)return false;
  for(unsigned i=0;i<4;++i)if(i!=slot&&bindings_[group][i]==instrument)return false;
  bindings_[group][slot]=instrument;return true;
 }
 int binding(unsigned group,unsigned slot)const noexcept {return group<4&&slot<4?bindings_[group][slot]:-1;}
 bool configureBase(unsigned pool,unsigned instrument,int config,const Values& values) noexcept {
  if(pool>=2||instrument>=4||config<1||config>127||!SourcePatchBank::validate(values))return false;
  auto& record=records_[4*pool+instrument];
  if(record.base.valid&&record.base.config!=config)return false;
  if(record.base.revision>=16777212u)return false;
  record.base.valid=true;record.base.config=config;record.base.values=values;++record.base.revision;return true;
 }
 bool base(unsigned pool,unsigned instrument,int config,Values& out)const noexcept {
  if(pool>=2||instrument>=4)return false;
  const auto& b=records_[4*pool+instrument].base;
  if(!b.valid||b.config!=config)return false;
  out=b.values;return true;
 }
 bool patch(unsigned pool,unsigned instrument,unsigned slot,Patch& out)const noexcept {
  if(pool>=2||instrument>=4||slot>=3)return false;
  out=records_[4*pool+instrument].patches[slot];return out.valid;
 }
 bool save(unsigned pool,unsigned instrument,unsigned slot,int config,const Values& values) noexcept {
  if(pool>=2||instrument>=4||slot>=3||!SourcePatchBank::validate(values))return false;
  auto& record=records_[4*pool+instrument];
  if(!record.base.valid||record.base.config!=config)return false;
  auto& p=record.patches[slot];if(p.revision>=16777212u)return false;
  p.valid=true;p.config=config;p.values=values;++p.revision;return true;
 }
 bool setMode(unsigned pool,unsigned instrument,unsigned index,int mode) noexcept {
  if(pool>=2||instrument>=4||index>=24||mode<1||mode>4)return false;
  records_[4*pool+instrument].modes[index]=mode;return true;
 }
 int mode(unsigned pool,unsigned instrument,unsigned index)const noexcept {
  return pool<2&&instrument<4&&index<24?records_[4*pool+instrument].modes[index]:-2;
 }
 void remember(unsigned pool,unsigned instrument,const Values& current) noexcept {
  if(pool<2&&instrument<4)records_[4*pool+instrument].last=current;
 }
private:
 struct Record {Patch base{};std::array<Patch,3> patches{};std::array<int,24> modes{};Values last{};
  Record()noexcept{modes.fill(-2);last.fill(-2);}};
 std::array<Record,8> records_{};
 std::array<std::array<int,4>,4> bindings_{};
};
}
