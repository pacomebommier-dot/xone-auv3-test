#pragma once
// L01/09: bounded LIVE state from S3 @zq/@zc/@zs/@zb and S5 @ObserveProfile.
// Not an AUM provider-authentication implementation. The AUv3 adapter must verify
// the incoming port before calling backendInput(); channel is never port auth.
// State is diagnostic/host-independent; no MIDI, no sysEx, no file or audio I/O.
#include <array>
#include <cstddef>
#include <cstdint>
#include "FxS6.hpp"
namespace xone::l01 {
struct LiveBinding { int channel{-1}; int cc{-1}; int address{-1}; bool enabled{}; };
struct LiveRead { bool known{}; int value{-2}; };
struct BrowserSpotInput {
    bool active{}; int packedP{-1}; int q{-1}; int expectedP{-1}; int expectedQ{-1};
};
struct BrowserSpotChange { int presetAddress{-1}; int preset{-2}; int physical{-1}; };
struct BrowserSpotPlan { bool valid{}; std::size_t count{}; std::array<BrowserSpotChange,12> changes{}; };
class LiveState {
public:
    static constexpr int Unknown=-2;
    static constexpr std::size_t CellCount=2048, BindingCount=36;
    static constexpr std::uint32_t MaxRevision=16777213;
    struct SessionImage {
        std::array<std::int16_t,CellCount> desired{};
        std::uint32_t revision{};
        int schema{}, config{};
    };
    LiveState() noexcept { desired_.fill(Unknown); observed_.fill(Unknown); desiredConfig_.fill(Unknown); }
    // Config/port provenance is supplied by a future verified host adapter.
    // We do not invent empty default bindings or recognize by MIDI channel alone.
    bool configureBackend(int config,std::uint32_t portToken,
                          const std::array<LiveBinding,BindingCount>& bindings) noexcept {
        if(config<1||config>127||portToken==0)return false;
        for(std::size_t i=0;i<bindings.size();++i){
            const auto& b=bindings[i];if(!b.enabled)continue;
            if(b.channel<0||b.channel>15||b.cc<0||b.cc>127||b.address<0||b.address>=static_cast<int>(CellCount))return false;
            for(std::size_t j=0;j<i;++j){const auto& v=bindings[j];if(!v.enabled)continue;
                if((v.channel==b.channel&&v.cc==b.cc)||v.address==b.address)return false;
            }
        }
        bindings_=bindings;backendConfig_=config;backendPort_=portToken;
        observed_.fill(Unknown);observationEpoch_=0;fxObservationEpoch_=0;
        ++bindingEpoch_;return true;
    }
    void disconnectBackend() noexcept {backendConfig_=-2;backendPort_=0;observed_.fill(Unknown);observationEpoch_=0;fxObservationEpoch_=0;++bindingEpoch_;}
    // The upstream AUM adapter must authenticate source port. Any absent or
    // ambiguous configuration is rejected. A stale epoch cannot overwrite newer data.
    bool backendInput(std::uint32_t portToken,int channel,int command,int cc,int value,
                      std::uint32_t epoch) noexcept {
        if(backendPort_==0||portToken!=backendPort_||backendConfig_<1||
           channel<0||channel>15||command!=0xB0||cc<0||cc>127||value<0||value>127||
           epoch==0||epoch>MaxRevision||epoch<observationEpoch_)return false;
        int address=-1;int matches=0;
        for(const auto& b:bindings_)if(b.enabled&&b.channel==channel&&b.cc==cc){++matches;address=b.address;}
        if(matches!=1||address<0)return false;
        observed_[static_cast<std::size_t>(address)]=static_cast<std::int16_t>(value);
        observationEpoch_=epoch;return true;
    }
    // Only native single-writer diagnostics may propose a logical/desired LIVE
    // value. It says nothing about actual backend MIDI response.
    bool commitDesired(int address,int value,std::uint32_t expectedRevision,int verifiedProfile=-2) noexcept {
        if(address<0||address>=static_cast<int>(CellCount)||value<0||value>127||
           expectedRevision!=revision_||revision_>=MaxRevision||
           (verifiedProfile!=-2&&(verifiedProfile<1||verifiedProfile>127)))return false;
        desired_[static_cast<std::size_t>(address)]=static_cast<std::int16_t>(value);
        desiredConfig_[static_cast<std::size_t>(address)]=static_cast<std::int8_t>(verifiedProfile);
        ++revision_;return true;
    }
    // Explicit recall of a validated 9-field FX MEMORY image into desired LIVE.
    // This is NOT an observed state, MIDI emission, PATCH write or AUM restore.
    // A whole image is one logical mutation with a single revision increment.
    bool commitFxMemoryImage(int owner,const std::array<int,9>& values,
                             std::uint32_t expectedRevision,int verifiedProfile) noexcept {
        if(owner<0||owner>=28||verifiedProfile<1||verifiedProfile>127||
           backendConfig_!=verifiedProfile||expectedRevision!=revision_||
           revision_>=MaxRevision)return false;
        std::array<int,9> addresses{};
        for(int i=0;i<9;++i){int v=values[static_cast<std::size_t>(i)];
            const int a=FxS6::addressFor(owner,i);
            if(a<0||a>=static_cast<int>(CellCount)||v<0||v>127||(i>=6&&v>1))return false;
            addresses[static_cast<std::size_t>(i)]=a;
        }
        for(int i=0;i<9;++i){auto j=static_cast<std::size_t>(i);
            desired_[static_cast<std::size_t>(addresses[j])]=static_cast<std::int16_t>(values[j]);
            desiredConfig_[static_cast<std::size_t>(addresses[j])]=static_cast<std::int8_t>(verifiedProfile);
        }
        ++revision_;return true;
    }
    bool commitPlannedSpots(const BrowserSpotPlan& p,std::uint32_t expectedRevision) noexcept {
        if(!p.valid||p.count>p.changes.size()||p.count==0||revision_!=expectedRevision||
           revision_>=MaxRevision)return false;
        for(std::size_t i=0;i<p.count;++i){const auto& c=p.changes[i];
            if(c.presetAddress<0||c.presetAddress>=static_cast<int>(CellCount)||c.preset<1||c.preset>4)return false;
            for(std::size_t j=0;j<i;++j)if(p.changes[j].presetAddress==c.presetAddress)return false;
        }
        // Validate entire batch before a single atomic commit/revision.
        for(std::size_t i=0;i<p.count;++i)desired_[static_cast<std::size_t>(p.changes[i].presetAddress)]=
            static_cast<std::int16_t>(p.changes[i].preset);
        ++revision_;return true;
    }
    bool restoreSession(const SessionImage& img) noexcept {
        if(img.schema!=1||img.config<1||img.config>127||img.revision<1||img.revision>MaxRevision)return false;
        for(auto v:img.desired)if(v!=Unknown&&(v<0||v>127))return false;
        // A valid session image is NOT evidence that the backend now matches.
        desired_=img.desired;revision_=img.revision;sessionConfig_=img.config;
        for(std::size_t n=0;n<CellCount;++n)desiredConfig_[n]=img.desired[n]>=0?static_cast<std::int8_t>(img.config):static_cast<std::int8_t>(Unknown);
        observed_.fill(Unknown);observationEpoch_=0;fxObservationEpoch_=0;return true;
    }
    SessionImage snapshot()const noexcept {SessionImage image;image.desired=desired_;image.revision=revision_;image.config=sessionConfig_;image.schema=1;return image;}
    // Source @zs via @wu; selector p encodes d6/t/s and q a SPOT offset.
    // No on/off, preset, or routing is synthesized from missing LIVE values.
    static bool spotAddresses(int packedP,int q,int& stateAddress,int& presetAddress) noexcept {
        stateAddress=-1;presetAddress=-1;
        if(packedP<0||q<0)return false;
        const int d6=packedP/1024,t=(packedP%1024)/32,s=packedP%32;
        if(d6!=3&&d6!=24)return false;
        if(t==1&&s>0&&s<9&&q<12){stateAddress=40*(s-1)+16+q;presetAddress=stateAddress+12;}
        else if(t==2&&s>0&&s<25&&q<12){stateAddress=320+42*(s-1)+18+q;presetAddress=stateAddress+12;}
        else if(d6==3&&t>2&&t<7&&s>0&&s<9&&q<3){stateAddress=1768+24*(t-3)+3*(s-1)+q;}
        else if(d6==3&&t>8&&t<13&&s>0&&s<4&&q<3){stateAddress=1477+9*(t-9)+3*(s-1)+q;}
        else if(d6==24&&t>13&&t<16&&s==0&&q<6){stateAddress=1864+6*(t-14)+q;}
        return stateAddress>=0&&stateAddress<static_cast<int>(CellCount);
    }
    LiveRead desired(int address) const noexcept {return read(desired_,address);}
    LiveRead observed(int address) const noexcept {return backendPort_!=0?read(observed_,address):LiveRead{};}
    // Tri-state (1 MATCH,0 DIFFERENT,-2 UNKNOWN). Never infer backend equality.
    int match(int address)const noexcept {
        const auto a=desired(address),b=observed(address);
        if(address<0||address>=static_cast<int>(CellCount)||backendConfig_<1||
           desiredConfig_[static_cast<std::size_t>(address)]!=backendConfig_)return Unknown;
        return a.known&&b.known?(a.value==b.value?1:0):Unknown;
    }
    BrowserSpotPlan planBrowserSpots(const std::array<BrowserSpotInput,12>& spots)const noexcept {
        BrowserSpotPlan p{};p.valid=true;
        for(std::size_t i=0;i<spots.size();++i){const auto& s=spots[i];if(!s.active)continue;
            int state=-1,preset=-1;
            if(s.packedP!=s.expectedP||s.q!=s.expectedQ||
               !spotAddresses(s.packedP,s.q,state,preset)||preset<0){p.valid=false;p.count=0;return p;}
            const auto on=desired(state),sel=desired(preset);
            if(!on.known||!sel.known||on.value<0||on.value>1||sel.value<1||sel.value>4){p.valid=false;p.count=0;return p;}
            // Source @zb f0=(p8%4)+1 after @zs established a valid pair.
            p.changes[p.count++]={preset,(sel.value%4)+1,static_cast<int>(i)};
        }
        // Reading this plan does not commit a change nor emit MIDI.
        return p;
    }
    // S6 FX status is a normalized observation, not a desired command.
    // The future AUv3 adapter has to verify that portToken identifies the
    // declared FX source. A channel-only or a stale stream is insufficient.
    bool ingestFxObserved(std::uint32_t portToken,int profile,int address,
                          bool known,int value,std::uint32_t epoch)noexcept {
        if(backendPort_==0||portToken!=backendPort_||profile!=backendConfig_||
           profile<1||epoch==0||epoch>MaxRevision||epoch<fxObservationEpoch_||
           address<0||address>=static_cast<int>(CellCount)||
           (known?(value<0||value>127):(value!=Unknown)))return false;
        observed_[static_cast<std::size_t>(address)]=known?static_cast<std::int16_t>(value):static_cast<std::int16_t>(Unknown);
        fxObservationEpoch_=epoch;return true;
    }
    void revokeFxObservations()noexcept {
        // FX ranges only, preserving non-FX MIDI observations.
        for(int i=1552;i<=1743;++i)observed_[static_cast<std::size_t>(i)]=Unknown;
        for(int i=1768;i<=1863;++i)observed_[static_cast<std::size_t>(i)]=Unknown;
        fxObservationEpoch_=0;
    }
    std::uint32_t fxObservationEpoch()const noexcept{return fxObservationEpoch_;}
    void onHostStop()noexcept{} // Do not confuse STOP with a device reset or a new observation.
    std::uint32_t revision()const noexcept{return revision_;}
    std::uint32_t observationEpoch()const noexcept{return observationEpoch_;}
    std::uint32_t bindingEpoch()const noexcept{return bindingEpoch_;}
    int backendConfig()const noexcept{return backendConfig_;}
    int sessionConfig()const noexcept{return sessionConfig_;}
private:
    static LiveRead read(const std::array<std::int16_t,CellCount>& a,int address) noexcept {
        if(address<0||address>=static_cast<int>(CellCount))return {};
        int v=a[static_cast<std::size_t>(address)];return {v>=0&&v<128,v};
    }
    std::array<std::int16_t,CellCount> desired_{},observed_{};
    std::array<std::int8_t,CellCount> desiredConfig_{};
    std::array<LiveBinding,BindingCount> bindings_{};
    int backendConfig_{Unknown},sessionConfig_{Unknown};
    std::uint32_t backendPort_{},bindingEpoch_{},observationEpoch_{},fxObservationEpoch_{},revision_{};
};
} // namespace xone::l01
