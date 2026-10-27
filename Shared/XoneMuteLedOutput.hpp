#pragma once
// Physical encoding from frozen L10 S7 @LedInitBindings/@LedRenderAll:
// row-major BASE, MIDI15 (index14), red/+36 amber/+72 green, 100ms phase.
// Owns only the 16 BASE LEDs while MUTE is visible; exit clears that image.
#include "XoneMuteLedView.hpp"
#include "../Core/NativeSlices.hpp"
namespace xone::host {
class MuteLedOutput {
public:
 struct Packet {bool valid{};unsigned target{},color{};bool on{};std::uint8_t bytes[3]{};};
 void beginBuffer() noexcept {failed_.fill(false);}
 void observe(const GlobalMuteState& mute,bool visible,std::int64_t ms) noexcept {
  if(visible&&!visible_){known_.fill(0);owned_=true;}
  visible_=visible;
  if(!owned_)return;
  const auto image=MuteLedView::compose(mute);
  for(unsigned i=0;i<16;++i){
   const auto led=image[i];
   const unsigned color=static_cast<unsigned>(led.color);
   const bool lit=visible && (!led.blinking || (ms>=0 && (ms/100)%2==0));
   desired_[i]=lit&&color>0&&color<4?static_cast<std::uint8_t>(1u<<(color-1)):0;
  }
 }
 Packet next(unsigned& cursor) const noexcept {
  if(!owned_)return {};
  while(cursor<16){
   const unsigned i=cursor;
   if(failed_[i]){++cursor;continue;}
   // Purge each proven color once on entry or uncertain callback. All OFFs
   // precede the desired ON; successful packets alone update the cache.
   for(unsigned c=0;c<3;++c){
    const unsigned bit=1u<<c;
    if(!(known_[i]&bit) || ((sent_[i]&bit)&&!(desired_[i]&bit)))return packet(i,c,false);
   }
   for(unsigned c=0;c<3;++c)if((desired_[i]&(1u<<c))&&!(sent_[i]&(1u<<c)))return packet(i,c,true);
   ++cursor;
  }
  return {};
 }
 void confirm(const Packet& p,bool success) noexcept {
  if(!p.valid||p.target>=16||p.color>=3)return;
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
 std::array<std::uint8_t,16> desired_{},sent_{},known_{};
 std::array<bool,16> failed_{};
 bool visible_{},owned_{};
};
}