#pragma once
// User-authorized working assumption (2026-11-04), NOT a qualified capture.
// See Profiles/SH01A_PATCH36_PROVISIONAL_V1.json for sources and exclusions.
// Explicit destination/channel/route/window remain required; never auto-enable.
#include "XoneSh01aHostBridge.hpp"
namespace xone::host {
inline constexpr std::array<uint8_t,36> Sh01aProvisionalCc36{{3,5,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,71,72,73,74,75,78,79,80,81,82,83,85,86,87}};
inline Sh01aProfile provisionalSh01aProfile(uint32_t destination,uint8_t channel,
        uint8_t cable,uint32_t windowMs)noexcept {
 Sh01aProfile p;p.destination=destination;p.channel=channel;p.cable=cable;
 p.windowMs=windowMs;p.cc=Sh01aProvisionalCc36;return p;
}
// Minimal AUM receipt mapping from V27: the three MACHINE FILTER controls.
// Other proposed V27 assignments/discrete encodings remain unbound, never guessed.
inline Sh01aAttachment provisionalSh01aAttachment(uint8_t source,int config,int alternative,
        uint32_t destination,uint8_t channel,uint8_t cable,uint32_t windowMs)noexcept {
 Sh01aAttachment a;a.enabled=true;a.source=source;a.config=config;a.alternative=alternative;
 a.image.profile=provisionalSh01aProfile(destination,channel,cable,windowMs);
 for(unsigned i=0;i<36;++i){const auto cc=a.image.profile.cc[i];
  if(cc==74)a.fields[i]=15;else if(cc==71)a.fields[i]=16;else if(cc==25)a.fields[i]=17;
 }
 return a;
}
}
