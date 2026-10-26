#pragma once
// L01 candidate slices: derived from frozen L10 S1/S2/S5/S6/S7/S8; NOT the full XONE engine.
// No alloc, MIDI send, AUM host calls, or external side effects in these handlers.
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::l01 {
enum class SurfaceKind : std::uint8_t { None, Edge, Dj, Cc };
struct MidiInput { int channel{}, command{}, number{}, value{}; }; // command 0x80,0x90,0xB0, channel 0-based
struct SurfaceResult {
    SurfaceKind kind{SurfaceKind::None};
    int control{-1}, phase{}, number{}, value{};
};
class SurfaceInput {
public:
    SurfaceInput() noexcept = default;
    explicit SurfaceInput(std::array<bool,36> persistedHeld) noexcept : held_(persistedHeld) {}
    const std::array<bool,36>& heldState() const noexcept {return held_;}
    // From L10 S1 @MapButton (four physical note bands). -1 means unmapped.
    static int physicalControl(int note) noexcept {
        if(note>=12 && note<16) return note-12;
        if(note>=24 && note<40) return 4 + 4*((39-note)/4) + note%4;
        if(note>=40 && note<52) return 20 + 4*((51-note)/4) + note%4;
        if(note>=52 && note<56) return 32 + note-52;
        return -1;
    }
    SurfaceResult accept(MidiInput m) noexcept {
        if(m.channel !=14 || m.number<0 || m.number>127 || m.value<0 || m.value>127)
            return {};
        if(m.command == 0x90 || m.command==0x80) {
            const int control=physicalControl(m.number);
            if(control<0) return {};
            const bool phase=(m.command==0x90 && m.value!=0);
            if(held_[static_cast<std::size_t>(control)]==phase) return {};
            held_[static_cast<std::size_t>(control)]=phase;
            return {SurfaceKind::Edge,control,phase?1:0,m.number,m.value};
        }
        if(m.command == 0xB0) {
            if(m.number<=19) return {SurfaceKind::Cc,-1,0,m.number,m.value};
            if(m.number<=21) return {SurfaceKind::Dj,-1,0,m.number,m.value};
        }
        return {};
    }
    // SOURCE: STOP must not fabricate physical UP; Held survives.
    void onHostStop() noexcept {}
    bool held(std::size_t control) const noexcept {return control<held_.size()&&held_[control];}
private:
    std::array<bool,36> held_{};
};
struct PrivateFrame {
    bool valid{}; int type{},control{},value{};
};
inline PrivateFrame decodeS1Frame(const std::array<int,11>& f) noexcept {
    for(int b:f) if(b<0 || b>127) return {};
    if(f[0]!=125||f[1]!=88||f[2]!=75||f[3]!=50||f[4]!=7||
       f[5]!=1||f[6]!=1||f[7]!=2) return {};
    if(f[8]==2 && f[9]>=0 && f[9]<36 && (f[10]==0||f[10]==1))
        return {true,2,f[9],f[10]};
    if(f[8]==3 && f[9]>=20 && f[9]<=21 && f[10]>=0 && f[10]<=127)
        return {true,3,f[9],f[10]};
    return {};
}
class RawGestureIngress { // only @AdmitEdge exact raw forwarding; not full S2 combos
public:
    bool accept(PrivateFrame f) noexcept {
        if(!f.valid) {++rejected_;return false;}
        if(f.type==3) return true;
        const auto i=static_cast<std::size_t>(f.control);
        if(f.value==1) {
            if(held_[i]) return false;
            held_[i]=true;
            return true;
        }
        if(!held_[i]) {++orphanUps_;return true;} // S2 forwards orphan physical UP
        held_[i]=false;
        return true;
    }
    void onHostStop() noexcept {} // real UP must remain possible
    unsigned orphanUps() const noexcept { return orphanUps_; }
    unsigned rejected() const noexcept { return rejected_; }
private:
    std::array<bool,36> held_{};
    unsigned orphanUps_{},rejected_{};
};
struct BotQualification { bool shiftClosed{},longBegin{}; };
class BotClock { // S2 @AdvanceClock thresholds only, not SMART/SHIFT combo resolver
public:
    bool down(unsigned bot,std::int64_t atMs) noexcept {
        if(bot>=4||active_[bot]) return false;
        active_[bot]=true; start_[bot]=atMs; shifted_[bot]=false; long_[bot]=false;
        return true;
    }
    BotQualification advance(unsigned bot,std::int64_t now) noexcept {
        if(bot>=4||!active_[bot]||now<start_[bot]) return {};
        const auto elapsed=now-start_[bot]; BotQualification q;
        if((bot==1||bot==2)&&elapsed>89&&!shifted_[bot]) {shifted_[bot]=true;q.shiftClosed=true;}
        if(elapsed>199&&!long_[bot]) {long_[bot]=true;q.longBegin=true;}
        return q;
    }
    bool up(unsigned bot) noexcept {if(bot>=4||!active_[bot])return false;active_[bot]=false;return true;}
private:
    std::array<bool,4> active_{},shifted_{},long_{};
    std::array<std::int64_t,4> start_{};
};
class EmptyObservationProfile { // S5 frozen empty @ObserveProfile
public:
    bool receive(int,int,int) noexcept {++rejected_;return false;}
    int config() const noexcept {return -2;}
    unsigned rejected() const noexcept {return rejected_;}
private:
    unsigned rejected_{};
};
// L10 S3 @w7 page/position index; identity intentionally distinct from BankOwnerId.
inline int memoryPageIndex(int page,int physicalPosition) noexcept {
    if((page==3||page==4)&&physicalPosition>=1&&physicalPosition<=8&&
       physicalPosition!=4&&physicalPosition!=8) {
        const int p=physicalPosition-1-(physicalPosition>4?1:0);
        return p+6*(page-3);
    }
    if((page==5||page==6)&&physicalPosition>=1&&physicalPosition<=8)
        return 12+8*(page-5)+physicalPosition-1;
    return -1;
}
struct MemoryRef {bool valid{};int record{},base{},count{};};
inline MemoryRef memoryIndex(int owner,int slot) noexcept {
    if(owner<0||owner>=28||slot<0) return {};
    int record{};
    if(owner<12) {if(slot>=4)return {};record=84+4*owner+slot;}
    else {if(slot>=3)return {};record=132+3*(owner-12)+slot;}
    return {true,record,9*(record-84),9};
}
struct BankLayout {bool valid{};int width{},fieldWidth{},modeWidth{},groups{},start{},count{};};
inline BankLayout s8BankLayout(int destination,int sub) noexcept {
    if(destination<0 || destination>10) return {};
    if(destination<8) {if(sub!=0)return {};return {true,60,30,24,1,0,60};}
    const int width=destination==8?28:30;
    if(sub==127)return {true,width,width,12,8,0,8*width};
    if(sub<0||sub>7)return {};
    return {true,width,width,12,1,sub,width};
}
enum class BankScan {Complete,Unknown,Corrupt};
inline BankScan validateS8Image(BankLayout l, const std::array<int,240>& values) noexcept {
    if(!l.valid||l.count<1||l.count>static_cast<int>(values.size()))return BankScan::Corrupt;
    bool unknown=false;
    for(int i=0;i<l.count;++i) {
        const int v=values[static_cast<std::size_t>(i)];
        if(v==-2){unknown=true;continue;}
        if(v<0||v>127)return BankScan::Corrupt;
        if(i%l.fieldWidth>=l.fieldWidth-12 && v>1)return BankScan::Corrupt;
    }
    return unknown?BankScan::Unknown:BankScan::Complete;
}
struct LedMidi {int offStatus{142},onStatus{158},note{-1};};
inline LedMidi ledBinding(int pos,int color) noexcept {
    if(pos<0||pos>=34||color<1||color>3)return {};
    if(pos==32) return color==1?LedMidi{142,158,12}:LedMidi{};
    if(pos==33)return LedMidi{142,158,color==1?15:(color==2?19:23)};
    int red{};
    if(pos<16)red=36-4*(pos/4)+pos%4;
    else if(pos<28){const int i=pos-16;red=48-4*(i/4)+i%4;}
    else red=52+pos-28;
    return {142,158,red+36*(color-1)};
}
class LedImage { // S7 76-byte atomic image + temporary flash status only
public:
    bool accept(const std::array<int,76>& f) noexcept {
        for(int x:f) if(x<0||x>127)return false;
        if(f[0]!=125||f[1]!=88||f[2]!=75||f[3]!=50||
           f[4]!=7||f[5]!=9||f[6]!=3||f[7]!=7)return false;
        std::array<int,34> colors{},patterns{};
        for(int i=0;i<34;++i){
            const int c=f[8+2*i],p=f[9+2*i];
            if(c>4||p>1||(c==4&&p!=0)||(i==32&&(c==2||c==3)))return false;
            colors[static_cast<std::size_t>(i)]=c==4?-2:c;
            patterns[static_cast<std::size_t>(i)]=p;
        }
        color_=colors;pattern_=patterns;flash_.fill(0);known_=true;
        return true;
    }
    void flash(int position,std::uint64_t expiry) noexcept {if(position>=0&&position<34)flash_[static_cast<std::size_t>(position)]=expiry;}
    void onHostStop() noexcept {flash_.fill(0);}
    bool known()const noexcept{return known_;}
    int color(std::size_t i)const noexcept{return color_[i];}
    int pattern(std::size_t i)const noexcept{return pattern_[i];}
    std::uint64_t flashUntil(std::size_t i)const noexcept{return flash_[i];}
private:
    bool known_{};
    std::array<int,34> color_{},pattern_{};
    std::array<std::uint64_t,34> flash_{};
};
}
