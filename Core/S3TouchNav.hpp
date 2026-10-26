#pragma once
// L01/08: source-bounded NAV TOUCH subset from frozen L10_S3 @z3/@zk/@zl/@zh.
// No MIDI/FX/LIVE writes. Enabled explicitly for diagnostics only.
// @zk k=2 operates only when no BOT1..3 owner, pending LONG or existing TOUCH.
// @zl: pending TOUCH for already-selected cells, short-release removal, 200-ms boundary.
#include "NavS3.hpp"
#include "GestureS2.hpp"
#include "S3DecisionBoundary.hpp"
#include <cstdint>
#include <cstddef>

namespace xone::l01 {
class S3TouchNav {
public:
    struct Stats {unsigned recognized{},commits{},deferred{},released{},rejected{},blocked{};};
    bool busy() const noexcept {return owner_!=-1;}
    int physicalOwner() const noexcept {return owner_;}
    Stats stats() const noexcept {return stats_;}
    void onHostStop() noexcept {clear();} // SOURCE @z5: no synthetic UP

    // Called on a staged copy of coordinator state so a failed MIDI packet
    // cannot leave either NAV or TOUCH partially mutated.
    bool process(const GestureBatch& batch,std::int64_t now,NavS3& nav,
                 const S3DecisionBoundary& s3,bool diagnostic,bool longPending) noexcept {
        if(now<0 || batch.count>batch.items.size() || batch.overflow)return false;
        for(std::size_t i=0;i<batch.count;++i){
            const auto&e=batch.items[i];
            if(e.kind!=GestureEventKind::RawEdge || e.control<4 || e.control>19)continue;
            if(e.value==1){
                if(diagnostic)onDown(e.control,now,nav,s3,longPending);
            }else if(e.value==0 && owner_==e.control){
                onUp(now,nav);
            }
        }
        return true;
    }
private:
    void clear() noexcept {owner_=-1;start_=0;group_=-1;primary_=-1;flag_=-1;page_=0;}
    void onDown(int physical,std::int64_t now,NavS3& nav,const S3DecisionBoundary& s3,bool longPending) noexcept {
        if(!nav.stable()||nav.hasStage()||longPending||busy()||s3.physicalOwner(1)!=-1||s3.physicalOwner(2)!=-1||s3.physicalOwner(3)!=-1){++stats_.blocked;return;}
        const auto route=nav.active().route(physical);
        if(route.address<0||route.address/1024!=2)return; // @zk k=2 only
        const int rem=route.address%1024,group=rem/32,value=rem%32,page=nav.active().at(291);
        int primary=-1,flag=-1;
        if(group==1)primary=313;
        else if(group==2)primary=314;
        else if(group>=9 && group<=12){primary=305+group-9;flag=309+group-9;}
        else if(group==3+page){primary=293+2*page;flag=299+2*page;
            if(value>4){++primary;++flag;}}
        else if(group==5+page){primary=297+page;flag=303+page;}
        else return; // no sourced NAV header mapping
        ++stats_.recognized;
        const auto&current=nav.active();
        bool same=(flag==-1)?current.at(primary)==value:
                         (current.at(flag)>0&&current.at(primary)==value);
        if(group>8 && same){
            for(int rankIdx=309;rankIdx<313;++rankIdx){
                if(rankIdx==flag||current.at(rankIdx)<=0||current.at(rankIdx)>=current.at(flag))continue;
                if(rankIdx==312||flag==312||(rankIdx<311&&flag<311))same=false;
            }
        }
        if(same){ // @zk a4=1,h1=1,h3=a9, awaiting @zl
            owner_=physical;start_=now;group_=group;primary_=primary;flag_=flag;page_=page;
            ++stats_.deferred;return;
        }
        NavImage next=current;
        if(group>=9&&group<=12){ // source @zh: reorder priorities 1..4
            int old=next.at(flag);if(old==0)old=5;
            for(int index=309;index<313;++index){
                if(next.at(index)>0 && next.at(index)<old)++next.at(index);
            }
            next.at(flag)=1;
        }else if(group>=3&&group<=6){
            // SOURCE @zk clears page-dependent flags before selecting header slot.
            next.at(299+2*page)=0;next.at(300+2*page)=0;next.at(303+page)=0;
            next.at(flag)=1;
        }
        next.at(primary)=value;
        if(group==2)next.at(315)=value>16?1:0;
        if(!NavS3::regenerate(next)||!NavS3::validate(next)||!nav.stage(next)||!nav.commit()){
            ++stats_.rejected;return;
        }
        ++stats_.commits;
    }
    void onUp(std::int64_t now,NavS3& nav) noexcept {
        // @zl: a4=1,h1=1,h3=a9. SOURCE v2=200 and strict elapsed <200.
        if(now>=start_ && now-start_<200 && flag_>=0 && nav.stable() && !nav.hasStage()){
            NavImage next=nav.active();
            if(group_>8){
                const int rank=next.at(flag_);next.at(flag_)=0;
                for(int j=309;j<313;++j)if(next.at(j)>rank && rank>0)--next.at(j);
            }else{
                next.at(299+2*page_)=0;next.at(300+2*page_)=0;next.at(303+page_)=0;
            }
            if(NavS3::regenerate(next) && NavS3::validate(next)&&nav.stage(next)&&nav.commit())++stats_.commits;
            else ++stats_.rejected;
        }
        ++stats_.released;clear();
    }
    int owner_{-1};std::int64_t start_{};int group_{-1},primary_{-1},flag_{-1},page_{};
    Stats stats_{};
};
}
