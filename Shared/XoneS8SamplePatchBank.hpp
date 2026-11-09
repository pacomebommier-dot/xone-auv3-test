#pragma once
// Native S8 SAMPLE instrument-only PATCH bank. No plugin, Roland program,
// audio or backend configuration is assumed. Empty at construction.
// 16 local CC + 12 local SPOT on/off; 12 SPOT behavior modes are CONFIG,
// NOT part of PATCH/BASE/PRESET/RED. No implicit migration from S6.
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::host {
class S8SamplePatchBank {
public:
    static constexpr unsigned Voices=8, Slots=3, ParamCount=28, ModeCount=12;
    static constexpr int Unknown=-2;
    static constexpr std::uint32_t MaxRevision=16777213;
    using Values=std::array<int,ParamCount>;
    using Modes=std::array<int,ModeCount>;
    struct Patch {
        bool valid{};int config{};std::uint32_t revision{};Values values{};
        Patch()noexcept{values.fill(Unknown);}
    };
    S8SamplePatchBank()noexcept {
        for(auto& m:modes_)m.fill(Unknown); // mode values are never inferred
    }
    static bool validate(const Values& v)noexcept {
        for(unsigned i=0;i<ParamCount;++i)
            if(v[i]<0||v[i]>127||(i>=16&&v[i]>1))return false;
        return true;
    }
    bool save(unsigned voice,unsigned slot,int config,const Values& image,
              std::uint32_t expectedRevision)noexcept {
        if(voice>=Voices||slot>=Slots||config<1||config>127||!validate(image))return false;
        auto& p=patches_[voice*Slots+slot];
        if(p.revision!=expectedRevision||p.revision>=MaxRevision-1||
           (p.valid&&p.config!=config))return false;
        p.values=image;p.config=config;p.valid=true;++p.revision;return true;
    }
    bool read(unsigned voice,unsigned slot,Patch& out)const noexcept {
        if(voice>=Voices||slot>=Slots)return false;
        out=patches_[voice*Slots+slot];return true;
    }
    // Explicit user-selected mode configuration, separately owned by S8.
    // No PATCH operation in this type reads or writes this state.
    bool setMode(unsigned voice,unsigned spot,int mode)noexcept {
        if(voice>=Voices||spot>=ModeCount||mode<1||mode>4)return false;
        modes_[voice][spot]=mode;return true;
    }
    int mode(unsigned voice,unsigned spot)const noexcept {
        return voice<Voices&&spot<ModeCount?modes_[voice][spot]:Unknown;
    }
    // Explicit document fields, never object bytes/padding or output caches.
    template<class Archive> bool archive(Archive& a) noexcept {
        for(auto& p:patches_){a(p.valid);a(p.config);a(p.revision);a(p.values);
            if(p.valid){if(p.config<1||p.config>127||p.revision<1||p.revision>16777212u||!validate(p.values))return false;}
            else {if(p.revision!=0||p.config!=0)return false;for(int v:p.values)if(v!=-2)return false;}
        }
        a(modes_);for(const auto& row:modes_)for(int v:row)if(v!=-2&&(v<1||v>4))return false;
        return a.ok();
    }
private:
    std::array<Patch,Voices*Slots> patches_{};
    std::array<Modes,Voices> modes_{};
};
} // namespace xone::host
