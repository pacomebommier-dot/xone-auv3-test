#pragma once
// Native S8 MACHINE PATCH bank, 24 logical machine targets, 3 local slots.
// Exactly 18 continuous controls + 12 SPOT states per machine. The twelve
// MIX/TOGGLE/TOUCH/FLASH modes belong to per-target S8 configuration, NEVER
// to PATCH, BASE, PRESET or RED. No instrument profile is inferred.
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::host {
class S8MachinePatchBank {
public:
    static constexpr unsigned Machines=24,Slots=3,ParamCount=30,ModeCount=12;
    static constexpr int Unknown=-2;
    static constexpr std::uint32_t MaxRevision=16777213;
    using Values=std::array<int,ParamCount>;
    using Modes=std::array<int,ModeCount>;
    struct Patch {
        bool valid{};int config{};std::uint32_t revision{};Values values{};
        Patch()noexcept{values.fill(Unknown);}
    };
    S8MachinePatchBank()noexcept {for(auto& m:modes_)m.fill(Unknown);}
    static bool validate(const Values& values)noexcept {
        for(unsigned i=0;i<ParamCount;++i)
            if(values[i]<0||values[i]>127||(i>=18&&values[i]>1))return false;
        return true;
    }
    bool save(unsigned machine,unsigned slot,int config,const Values& image,
              std::uint32_t expectedRevision)noexcept {
        if(machine>=Machines||slot>=Slots||config<1||config>127||!validate(image))return false;
        auto& p=patches_[machine*Slots+slot];
        if(p.revision!=expectedRevision||p.revision>=MaxRevision-1||
           (p.valid&&p.config!=config))return false;
        p.values=image;p.config=config;p.valid=true;++p.revision;return true;
    }
    bool read(unsigned machine,unsigned slot,Patch& out)const noexcept {
        if(machine>=Machines||slot>=Slots)return false;
        out=patches_[machine*Slots+slot];return true;
    }
    bool setMode(unsigned machine,unsigned spot,int mode)noexcept {
        if(machine>=Machines||spot>=ModeCount||mode<1||mode>4)return false;
        modes_[machine][spot]=mode;return true;
    }
    int mode(unsigned machine,unsigned spot)const noexcept {
        return machine<Machines&&spot<ModeCount?modes_[machine][spot]:Unknown;
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
    std::array<Patch,Machines*Slots> patches_{};
    std::array<Modes,Machines> modes_{};
};
} // namespace xone::host
