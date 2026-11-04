#pragma once
#include "XoneSh01aMirror.hpp"
#include "XoneSourcePatchBank.hpp"
namespace xone::host {
// One explicitly configured hardware destination. Its identity lives with its
// three mirrors; changing source or MIDI route cannot silently relabel a bank.
struct Sh01aAttachment {
 bool enabled{};uint8_t source{};int config{1},alternative{-1};
 Sh01aMirrorImage image{};std::array<int16_t,36> fields{};
 Sh01aAttachment()noexcept{fields.fill(-1);}
 bool valid()const noexcept {
  if(!enabled)return true;
  // Current AU output cable0 also carries universal S7 (channels7..14)
  // and K2 LED15. A shared channel cannot identify the SH destination.
  if(image.profile.channel>=6&&image.profile.channel<=14)return false;
  if(source>=4||config<1||config>127||alternative < -1||alternative>3||!image.valid())return false;
  std::array<bool,60> used{};
  for(auto f:fields){if(f==-1)continue;
   // Central controls on even MACHINE pages are FX aliases, never SH bindings.
   if(!((f>=0&&f<18)||(f>=42&&f<48))||used[static_cast<unsigned>(f)])return false;
   used[static_cast<unsigned>(f)]=true;
  }
  return true;
 }
 int cell(unsigned index)const noexcept{return index<36&&fields[index]>=0?SourcePatchBank::cell(source,static_cast<unsigned>(fields[index])):-1;}
 int api(unsigned index)const noexcept{return index<36&&fields[index]>=0?SourcePatchBank::api(source,static_cast<unsigned>(fields[index])):-1;}
 int indexForCell(int cellId)const noexcept {
  for(unsigned i=0;i<36;++i)if(fields[i]>=0&&cell(i)==cellId)return static_cast<int>(i);
  return -1;
 }
};
using Sh01aAttachmentBytes=std::array<uint8_t,12+Sh01aMirrorBytes{}.size()+36+4>;
inline bool encodeSh01aAttachment(const Sh01aAttachment& a,Sh01aAttachmentBytes& out)noexcept {
 if(!a.valid())return false;
 Sh01aAttachmentBytes b{};b[0]='S';b[1]='H';b[2]='A';b[3]='U';b[4]=1;b[5]=a.enabled?1:0;
 if(a.enabled){
  b[8]=a.source;b[9]=static_cast<uint8_t>(a.config);b[10]=static_cast<uint8_t>(a.alternative+1);
  Sh01aMirrorBytes inner{};if(!encodeSh01aMirror(a.image,inner))return false;
  unsigned at=12;for(auto v:inner)b[at++]=v;
  for(auto f:a.fields)b[at++]=static_cast<uint8_t>(f+1);
 }
 sessionPut32(b.data()+b.size()-4,sessionCrc(b.data(),b.size()-4));out=b;return true;
}
inline bool decodeSh01aAttachment(const uint8_t* b,std::size_t size,Sh01aAttachment& out)noexcept {
 if(!b||size!=Sh01aAttachmentBytes{}.size()||b[0]!='S'||b[1]!='H'||b[2]!='A'||b[3]!='U'||b[4]!=1||b[5]>1||b[6]||b[7]||b[11]||sessionGet32(b+size-4)!=sessionCrc(b,size-4))return false;
 Sh01aAttachment a;a.enabled=b[5]!=0;
 if(a.enabled){
  a.source=b[8];a.config=b[9];a.alternative=static_cast<int>(b[10])-1;
  if(!decodeSh01aMirror(b+12,Sh01aMirrorBytes{}.size(),a.image))return false;
  for(unsigned i=0;i<36;++i)a.fields[i]=static_cast<int16_t>(static_cast<int>(b[12+Sh01aMirrorBytes{}.size()+i])-1);
 }else for(unsigned i=8;i<size-4;++i)if(b[i])return false;
 if(!a.valid())return false;
 out=a;return true;
}
class Sh01aHostBridge {
public:
 struct Frame {Sh01aAttachment attachment{};uint32_t serial{},restoreToken{};};
 struct Packet {bool valid{};uint64_t token{};int index{-1},value{-1};unsigned length{};std::array<uint8_t,3> bytes{};};
 bool request(const Sh01aAttachment& a,uint32_t restoreToken=0)noexcept {
  if(!a.valid()||requested_==UINT32_MAX)return false;
  ui_={a,++requested_,restoreToken};requests_.publish(ui_);return true;
 }
 Sh01aAttachment forHost()noexcept {
  Frame next;if(snapshots_.take(next)&&next.serial>=requested_)ui_=next;
  return ui_.attachment;
 }
 bool hasPending()const noexcept{return pendingValid_||requests_.hasUpdate();}
 int pendingProgram()const noexcept{return pendingProgram_;}
 bool beginBuffer(bool safe,uint32_t appliedRestore)noexcept {
  blocked_=false;
  if(requests_.take(pending_))pendingValid_=true;
  if(!pendingValid_||!safe||(pending_.restoreToken&&pending_.restoreToken!=appliedRestore))return false;
  attachment_=pending_.attachment;serial_=pending_.serial;pendingValid_=false;
  mirror_=Sh01aMirror{};
  if(attachment_.enabled)mirror_.restore(attachment_.image);
  clearOutput();dirty_=true;return true;
 }
 void endBuffer()noexcept {
  if(!dirty_)return;
  if(attachment_.enabled)attachment_.image=mirror_.snapshot();
  snapshots_.publish({attachment_,serial_,0});dirty_=false;
 }
 const Sh01aAttachment& attachment()const noexcept{return attachment_;}
 const Sh01aMirror& mirror()const noexcept{return mirror_;}
 Sh01aMirror::Observation receive(uint8_t cable,const uint8_t* bytes,std::size_t size,int64_t now)noexcept {
  auto result=mirror_.receive(cable,bytes,size,now);
  if(result.mirrorReplaced){attachment_.image=mirror_.snapshot();dirty_=true;}
  return result;
 }
 void advance(int64_t now)noexcept{mirror_.advance(now);}
 bool queueProgram(unsigned pc)noexcept {
  if(!attachment_.enabled||pc>=3||token_==UINT64_MAX)return false;
  pendingProgram_=static_cast<int>(pc);pendingCc_.fill(-2);mirror_.stop();++token_;return true;
 }
 bool localEdit(unsigned index,int value)noexcept {
  if(!attachment_.enabled||index>=36||token_==UINT64_MAX||!mirror_.localEdit(attachment_.image.profile.cc[index],value))return false;
  // A manual edit cannot be replayed after an older failed program selection.
  pendingProgram_=-1;pendingCc_[index]=value;++token_;return true;
 }
 Packet next()const noexcept {
  if(!attachment_.enabled||blocked_)return {};
  const auto channel=attachment_.image.profile.channel;
  if(pendingProgram_>=0)return {true,token_,-1,pendingProgram_,2,{{static_cast<uint8_t>(0xC0+channel),static_cast<uint8_t>(pendingProgram_),0}}};
  for(unsigned i=0;i<36;++i)if(pendingCc_[i]>=0)return {true,token_,static_cast<int>(i),pendingCc_[i],3,{{static_cast<uint8_t>(0xB0+channel),attachment_.image.profile.cc[i],static_cast<uint8_t>(pendingCc_[i])}}};
  return {};
 }
 bool acknowledge(const Packet& packet,bool success,int64_t now)noexcept {
  const auto current=next();
  if(!packet.valid||!current.valid||packet.token!=current.token||packet.index!=current.index||packet.value!=current.value)return false;
  if(!success){blocked_=true;return true;}
  if(packet.index<0){pendingProgram_=-1;mirror_.recallSent(static_cast<unsigned>(packet.value),now);}
  else pendingCc_[static_cast<unsigned>(packet.index)]=-2;
  return true;
 }
 void stop()noexcept{mirror_.stop();clearOutput();}
private:
 void clearOutput()noexcept{pendingProgram_=-1;pendingCc_.fill(-2);}
 StateMailbox<Frame> requests_{},snapshots_{};Frame ui_{},pending_{};uint32_t requested_{},serial_{};
 bool pendingValid_{},dirty_{},blocked_{};Sh01aAttachment attachment_{};Sh01aMirror mirror_{};
 int pendingProgram_{-1};std::array<int,36> pendingCc_{};uint64_t token_{};
};
}
