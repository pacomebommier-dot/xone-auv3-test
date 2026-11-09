#pragma once
// L01/04: qualification S2 issue du texte figé L10_S2_NONCURRENT.txt,
// @AdmitEdge @BotPhysicalDown/Up @FindPairRole @DJPhysical @AdvanceClock @OnHostStop.
// Pas de décision NAV/LIVE; qualifications brutes à valider par S3.
#include "NativeSlices.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::l01 {
enum class QualifiedPhase : std::uint8_t { Short=0, LongBegin=1, LongRelease=2, ComboDown=3, ShiftClosed=4 };
enum class GestureEventKind : std::uint8_t { RawEdge, RawDj, RawCc, Qualified };
struct GestureEvent {GestureEventKind kind{};int control{-1};int value{};int role{-1};QualifiedPhase phase{};};
struct GestureBatch {
    std::array<GestureEvent,20> items{};std::size_t count{};bool overflow{};
    bool push(GestureEvent event) noexcept {if(count>=items.size()){overflow=true;return false;}items[count++]=event;return true;}
};
class GestureS2 {
public:
    explicit GestureS2(std::array<bool,36> held={}) noexcept : held_(held) {
        for(int i=0;i<36;++i)if(held_[i]){pressControl_[i]=i;if(i<4)bot_[i].state=4;}
    }
    // advance-before-input reproduit S2 @OnSysex/@OnMidiInput; refus temps retrograde.
    bool accept(const SurfaceResult& raw,std::int64_t now,GestureBatch& out) noexcept {
        if(now<0 || (hasClock_&&now<lastMs_)||out.count+10>out.items.size())return false;
        // Refuse invalid or saturated batches BEFORE qualifying time or mutating Held.
        if(raw.kind==SurfaceKind::Edge){if(raw.control<0||raw.control>=36||(raw.phase!=0&&raw.phase!=1))return false;}
        else if(raw.kind==SurfaceKind::Dj){if(raw.number<20||raw.number>21||raw.value<0||raw.value>127)return false;}
        else if(raw.kind==SurfaceKind::Cc){if(raw.number<0||raw.number>19||raw.value<0||raw.value>127)return false;}
        else if(raw.kind!=SurfaceKind::None)return false;
        GestureBatch stage{};
        if(!tick(now,stage))return false;
        if(raw.kind==SurfaceKind::Edge){
            if(raw.control<0||raw.control>=36||(raw.phase!=0&&raw.phase!=1))return false;
            const auto idx=static_cast<std::size_t>(raw.control);
            if(raw.phase==1){
                if(!held_[idx]){
                    held_[idx]=true;pressControl_[idx]=raw.control;
                    if(raw.control<4)botDown(raw.control,now,stage);
                    stage.push({GestureEventKind::RawEdge,raw.control,1,-1,{}});
                }
            } else {
                if(!held_[idx]){++orphanUps_;stage.push({GestureEventKind::RawEdge,raw.control,0,-1,{}});}
                else {
                    if(pressControl_[idx]==raw.control&&raw.control<4)botUp(raw.control,stage);
                    held_[idx]=false;pressControl_[idx]=-1;
                    stage.push({GestureEventKind::RawEdge,raw.control,0,-1,{}});
                }
            }
        } else if(raw.kind==SurfaceKind::Dj){
            if(raw.number<20||raw.number>21||raw.value<0||raw.value>127)return false;
            if(raw.number==21&&held_[2]&&bot_[2].state==1&&now-bot_[2].down>=0&&now-bot_[2].down<200){
                bot_[2].state=6;bot_[2].role=10;qualify(stage,10,QualifiedPhase::ComboDown);
            }
            stage.push({GestureEventKind::RawDj,raw.number,raw.value,-1,{}});
        } else if(raw.kind==SurfaceKind::Cc){
            if(raw.number<0||raw.number>19||raw.value<0||raw.value>127)return false;
            stage.push({GestureEventKind::RawCc,raw.number,raw.value,-1,{}});
        } else if(raw.kind!=SurfaceKind::None)return false;
        if(stage.overflow||stage.count+out.count>out.items.size())return false;
        for(std::size_t i=0;i<stage.count;++i)out.push(stage.items[i]);
        return true;
    }
    bool onTimer(std::int64_t now,GestureBatch& out) noexcept {
        if(now<0||(hasClock_&&now<lastMs_)||out.count+8>out.items.size())return false;
        GestureBatch stage{};if(!tick(now,stage)||stage.count+out.count>out.items.size())return false;
        for(std::size_t i=0;i<stage.count;++i)out.push(stage.items[i]);
        return true;
    }
    void onHostStop() noexcept {
        for(auto& b:bot_){if(b.state==2||b.state==8)b.state=5;
            else if(b.state!=6&&b.state>0)b.state=4;}
        // S2 @OnHostStop: never create UP, retains Held and elapsed origin.
    }
    bool held(std::size_t i)const noexcept{return i<held_.size()&&held_[i];}
    unsigned orphanUps()const noexcept{return orphanUps_;}
    bool timerNeeded()const noexcept {for(auto b:bot_)if(b.state==1||b.state==7)return true;return false;}
private:
    struct Bot {int state{},peer{-1},role{-1},owner{-1};bool shift{};std::int64_t down{};};
    static void qualify(GestureBatch& events,int role,QualifiedPhase p)noexcept {
        if(role>=0&&role<=10)events.push({GestureEventKind::Qualified,-1,0,role,p});
    }
    static int pairRole(int first,int second)noexcept {
        if(first==1){if(second==2)return 4;if(second==3)return 6;if(second==0)return 9;}
        if(first==2){if(second==1)return 5;if(second==3)return 7;if(second==0)return 8;}
        return -1;
    }
    bool tick(std::int64_t now,GestureBatch& batch)noexcept {
        lastMs_=now;hasClock_=true;
        for(int i=0;i<4;++i){auto& b=bot_[i];if(b.state!=1&&b.state!=7)continue;
            const auto age=now-b.down;
            if(b.state==1&&(i==1||i==2)&&!b.shift&&age>89){
                b.shift=true;qualify(batch,i,QualifiedPhase::ShiftClosed);
            }
            if(age>199){const int old=b.state;b.state=old==1?2:8;qualify(batch,b.role,QualifiedPhase::LongBegin);}
        }
        return !batch.overflow;
    }
    void botDown(int i,std::int64_t now,GestureBatch& out)noexcept {
        auto& b=bot_[i];b={};b.down=now;b.role=i;b.owner=i;
        int first=-1;int role=-1;std::int64_t candidate=now+1;
        for(int p=0;p<4;++p){const auto& probe=bot_[p];
            if(p==i||probe.state!=1)continue;
            const auto age=now-probe.down;
            if(age>=0&&age<90){const int r=pairRole(p,i);
                if(r>=0&&probe.down<candidate){first=p;role=r;candidate=probe.down;}}
        }
        if(first>=0){bot_[first].state=3;b.state=7;bot_[first].peer=i;b.peer=first;
            bot_[first].role=role;b.role=role;bot_[first].owner=i;b.owner=i;
            qualify(out,role,QualifiedPhase::ComboDown);
        }else{
            b.state=1;
            if(i!=0){for(int p=0;p<4;++p){if(p==i)continue;
                if(p!=0||bot_[p].state==7||bot_[p].state==8){
                    const int s=bot_[p].state;
                    if(s==1||s==2||s==7||s==8)b.state=4;
                }
            }}
        }
    }
    void botUp(int i,GestureBatch& out)noexcept {
        auto& b=bot_[i];
        if(b.state==1||b.state==7)qualify(out,b.role,QualifiedPhase::Short);
        else if(b.state==2||b.state==8||b.state==5)qualify(out,b.role,QualifiedPhase::LongRelease);
        else if(b.state==6)qualify(out,10,QualifiedPhase::LongRelease);
        b={};
    }
    std::array<bool,36> held_{};
    std::array<int,36> pressControl_=[]{std::array<int,36> a{};a.fill(-1);return a;}();
    std::array<Bot,4> bot_{};
    unsigned orphanUps_{};bool hasClock_{};std::int64_t lastMs_{};
};
}
