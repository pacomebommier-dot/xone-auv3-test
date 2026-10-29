#pragma once
// Physical encoding from frozen L10 S7 @LedInitBindings/@LedRenderAll:
// row-major BASE, MIDI15 (index14), red/+36 amber/+72 green, 100ms phase.
// One physical cache for all 34 positions; legacy BASE views use its first16.
#include "XoneBaseLedView.hpp"
#include "../Core/NativeSlices.hpp"
namespace xone::host {
class MuteLedOutput {
public:
 using Image=std::array<BaseLed,34>;
 struct Packet {bool valid{};unsigned target{},color{};bool on{};std::uint8_t bytes[3]{};};
 void invalidate() noexcept {known_.fill(0);until_.fill(0);owned_=true;}
 void cancelFlashes() noexcept {until_.fill(0);}
 void flash(unsigned position,MuteLedColor color,std::int64_t now) noexcept {
  const auto c=static_cast<unsigned>(color);
  if(position>=32||c<1||c>3||now<0||now>INT64_MAX-100)return;
  flash_[position]=static_cast<std::uint8_t>(1u<<(c-1));until_[position]=now+100;
 }
 void observeFull(const Image& context,std::int64_t ms) noexcept {
  owned_=true;count_=34;bool changed=false;
  for(unsigned i=0;i<34;++i)if(context[i].known!=previous_[i].known||context[i].color!=previous_[i].color||context[i].blinking!=previous_[i].blinking)changed=true;
  if(changed)cancelFlashes();
  previous_=context;
  for(unsigned i=0;i<34;++i){const auto& led=context[i];const unsigned color=static_cast<unsigned>(led.color);
   const bool lit=led.known&&(!led.blinking||(ms>=0&&(ms/100)%2==0));
   desired_[i]=lit&&color>0&&color<4?static_cast<std::uint8_t>(1u<<(color-1)):0;
   if(ms>=0&&until_[i]>ms)desired_[i]=flash_[i];
  }
 }
 void beginBuffer() noexcept {failed_.fill(false);}
 void observe(const GlobalMuteState& mute,bool visible,std::int64_t ms,const BaseLedView::Image& context={}) noexcept {
  count_=16;
  if(visible&&!visible_){known_.fill(0);owned_=true;}
  visible_=visible;
  if(!owned_)return;
  const auto image=MuteLedView::compose(mute);
  for(unsigned i=0;i<16;++i){
   const auto led=visible?image[i]:MuteLed{context[i].color,context[i].blinking};
   const unsigned color=static_cast<unsigned>(led.color);
   const bool lit=(visible||context[i].known) && (!led.blinking || (ms>=0 && (ms/100)%2==0));
   desired_[i]=lit&&color>0&&color<4?static_cast<std::uint8_t>(1u<<(color-1)):0;
  }
 }
 Packet next(unsigned& cursor) const noexcept {
  if(!owned_)return {};
  while(cursor<count_){
   const unsigned i=cursor;
   if(failed_[i]){++cursor;continue;}
   // Purge each proven color once on entry or uncertain callback. All OFFs
   // precede the desired ON; successful packets alone update the cache.
   for(unsigned c=0;c<(i==32?1u:3u);++c){
    const unsigned bit=1u<<c;
    if(!(known_[i]&bit) || ((sent_[i]&bit)&&!(desired_[i]&bit)))return packet(i,c,false);
   }
   for(unsigned c=0;c<(i==32?1u:3u);++c)if((desired_[i]&(1u<<c))&&!(sent_[i]&(1u<<c)))return packet(i,c,true);
   ++cursor;
  }
  return {};
 }
 void confirm(const Packet& p,bool success) noexcept {
  if(!p.valid||p.target>=count_||p.color>=3)return;
  if(!success){known_[p.target]=0;failed_[p.target]=true;return;}
  const auto bit=static_cast<std::uint8_t>(1u<<p.color);
  known_[p.target]|=bit;
  if(p.on)sent_[p.target]|=bit;
  else sent_[p.target]&=static_cast<std::uint8_t>(~bit);
 }
private:
 static Packet packet(unsigned i,unsigned c,bool on) noexcept {
  const auto binding=l01::ledBinding(static_cast<int>(i),static_cast<int>(c+1));
  return {true,i,c,on,{static_cast<std::uint8_t>(on?binding.onStatus:binding.offStatus),
                      static_cast<std::uint8_t>(binding.note),static_cast<std::uint8_t>(on?127:0)}};
 }
 std::array<std::uint8_t,34> desired_{},sent_{},known_{};
 std::array<bool,34> failed_{};
 Image previous_{};std::array<std::int64_t,34> until_{};std::array<std::uint8_t,34> flash_{};unsigned count_{16};
 bool visible_{},owned_{};
};
}