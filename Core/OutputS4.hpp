#pragma once
// L01/04: projection diagnostique dérivée de L10_S4_NONCURRENT.txt
// @OnSysex 13 @OutIntent @OutEmit @OutOn @OutOff @OnHostStop.
// Aucun binding AUM, aucun MIDI physique: trace 3 octets destinée aux tests.
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::l01 {
struct LogicalIntent {int type{},address{},value{},key{127};};
struct MidiPacket {std::uint8_t status{},number{},value{};};
class OutputS4 {
public:
    static constexpr std::size_t Capacity=1408; // 702 debts + room for equivalent batch; bounded
    void setDiagnosticEnabled(bool enabled)noexcept {enabled_=enabled;}
    bool enabled()const noexcept{return enabled_;}
    std::size_t emitted()const noexcept{return count_;}
    MidiPacket packet(std::size_t n)const noexcept{return n<count_?packets_[n]:MidiPacket{};}
    void drain()noexcept{count_=0;}
    int outstanding()const noexcept {int n=0;for(bool d:debt_)if(d)++n;return n;}
    bool debt(int address)const noexcept{return address>=0&&address<702&&debt_[static_cast<std::size_t>(address)];}
    int owner(int key)const noexcept{return key>=0&&key<36?owner_[static_cast<std::size_t>(key)]:-1;}
    bool apply(LogicalIntent x)noexcept {
        if(!valid(x))return false;
        const int a=x.address; const int v=x.value; const int k=x.key;
        // Recheck the physical-key bound locally: L01/04 validated at entry,
        // but GCC -O2 cannot always prove the path from the inlined valid().
        if((x.type==12||x.type==13||x.type==14)&&(k<0||k>=36))return false;
        const auto ia=static_cast<std::size_t>(a);
        std::size_t needed=0;
        if(x.type==16||x.type==17){needed=enabled_?1:0;}
        else if(x.type==10||x.type==15){needed=enabled_&&ccSent_[ia]!=v?1:0;}
        else if(x.type==11||x.type==18){
            if(state_[ia]!=v || static_cast<int>(debt_[ia])!=v){
                if(v==1)needed=(enabled_&&!debt_[ia])?1:0;
                else if(!owned(a))needed=(debt_[ia]||enabled_)?1:0;
            }
        }else if(x.type==12){if(enabled_)needed=1;}
        else if(x.type==13){needed=debt_[ia]?1:0;
            if(state_[ia]==1&&enabled_)needed+=1;}
        else if(x.type==14){if(enabled_)needed=2;}
        // OutService kind19 has no MIDI binding in frozen S5.
        if(x.type==11&&v==1){for(int z:inhibitAddr_)if(z==a)return false;}
        if(x.type==12&&(!enabled_||inhibit_[k]||owner_[k]!=-1||debt_[ia]||state_[ia]==1))return false;
        if(x.type==14&&(inhibit_[k]||owner_[k]!=-1||debt_[ia]||state_[ia]==1))return false;
        if(x.type==13){if(inhibit_[k]){
                if(inhibitAddr_[k]==a){inhibit_[k]=false;inhibitAddr_[k]=-1;}return true;}
            if(owner_[k]!=a)return false;}
        if(!reserve(needed, x))return false;
        if(x.type==16||x.type==17){if(enabled_) emit(0xB0,a,v);}
        else if(x.type==19)service_[ia]=v;
        else if(x.type==10||x.type==15){
            if(enabled_&&ccSent_[ia]!=v){emit(0xB0,a,v);ccSent_[ia]=v;}
            cc_[ia]=v;
        }else if(x.type==11||x.type==18){
            if(state_[ia]!=v||static_cast<int>(debt_[ia])!=v){
                state_[ia]=v;
                if(v==1){if(!debt_[ia]&&enabled_){emit(0x90,a,127);debt_[ia]=true;}}
                else if(!owned(a)){
                    if(debt_[ia]){emit(0x80,a,0);debt_[ia]=false;}
                    else if(enabled_)emit(0x80,a,0);
                }
            }
        }else if(x.type==12){emit(0x90,a,127);debt_[ia]=true;owner_[k]=a;}
        else if(x.type==13){
            if(debt_[ia]){emit(0x80,a,0);debt_[ia]=false;}
            owner_[k]=-1;
            if(state_[ia]==1&&enabled_){emit(0x90,a,127);debt_[ia]=true;}
        }else if(x.type==14){emit(0x90,a,127);emit(0x80,a,0);}
        return true;
    }
    // Transactional staged output for a future qualified S3 decision. The caller
    // must supply an explicit known route; this never discovers a destination.
    // Diagnostic projection only, NOT host-authenticated AUM MIDI output.
    bool applyQualifiedBatch(const std::array<LogicalIntent,8>& intents,std::size_t n)noexcept {
        if(!enabled_||n==0||n>intents.size())return false;
        auto draft=*this;
        for(std::size_t i=0;i<n;++i)if(!draft.apply(intents[i]))return false;
        *this=draft;return true;
    }
    bool onHostStop()noexcept {
        const auto n=static_cast<std::size_t>(outstanding());
        if(count_+n>Capacity)return false;
        for(int a=0;a<702;++a)if(debt_[static_cast<std::size_t>(a)]){
            emit(0x80,a,0);debt_[static_cast<std::size_t>(a)]=false;
        }
        owner_.fill(-1);inhibit_.fill(false);inhibitAddr_.fill(-1);
        return true;
    }
private:
    bool valid(const LogicalIntent& x)const noexcept {
        if(x.address<0||x.value<0||x.value>127)return false;
        switch(x.type){
        case 10:return x.address<931&&(x.address<569||x.address>573)&&x.key==127;
        case 15:return x.address>=569&&x.address<=571&&x.key==127;
        case 16:return x.address>=947&&x.address<=948&&x.value>=1&&x.key==127;
        case 17:return x.address>=931&&x.address<=946&&(x.value==0||x.value==127)&&x.key==127;
        case 19:return x.address<2&&x.key==127&&(x.address==0?x.value<=1:x.value>=1&&x.value<=2);
        case 11:return x.address<544&&x.value<=1&&x.key==127;
        case 18:return x.address>=544&&x.address<608&&x.value<=1&&x.key==127;
        case 12: case 14:return x.address<702&&x.value==1&&x.key>=0&&x.key<36;
        case 13:return x.address<702&&x.value==0&&x.key>=0&&x.key<36;
        default:return false;
        }
    }
    bool owned(int address)const noexcept{for(int a:owner_)if(a==address)return true;return false;}
    bool reserve(std::size_t need, LogicalIntent x)const noexcept{
        if(count_+need>Capacity)return false;
        // Future STOP must be able to close all outstanding notes without losing OFF.
        const std::size_t extra=((x.type==12&&enabled_)||
          ((x.type==11||x.type==18)&&x.value==1&&enabled_&&!debt_[static_cast<std::size_t>(x.address)]))?1:0;
        const std::size_t release=((x.type==13&&debt_[static_cast<std::size_t>(x.address)])||
          ((x.type==11||x.type==18)&&x.value==0&&debt_[static_cast<std::size_t>(x.address)]&&!owned(x.address)))?1:0;
        return count_+need+static_cast<std::size_t>(outstanding())+extra-release<=Capacity;
    }
    void emit(int base,int address,int value)noexcept {
        const int status=base+6+address/128;
        packets_[count_++]={static_cast<std::uint8_t>(status),static_cast<std::uint8_t>(address%128),static_cast<std::uint8_t>(value)};
    }
    bool enabled_{};std::size_t count_{};
    std::array<MidiPacket,Capacity> packets_{};
    std::array<bool,702> debt_{};
    std::array<int,702> state_=[]{std::array<int,702>a{};a.fill(-1);return a;}();
    std::array<int,949> cc_=[]{std::array<int,949>a{};a.fill(-2);return a;}();
    std::array<int,949> ccSent_=[]{std::array<int,949>a{};a.fill(-2);return a;}();
    std::array<int,36> owner_=[]{std::array<int,36>a{};a.fill(-1);return a;}();
    std::array<bool,36> inhibit_{};
    std::array<int,36> inhibitAddr_=[]{std::array<int,36>a{};a.fill(-1);return a;}();
    std::array<int,2> service_{};
};
}
