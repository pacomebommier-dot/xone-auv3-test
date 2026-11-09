#pragma once
// Port of S7 S3 @wu/@zp/@zr; keeps source LIVE cell separate from API CC.
#include "../Core/NavS3.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace xone::host {
struct ContinuousTarget {int cell{-1},api{-1};bool valid()const noexcept{return cell>=0&&api>=0;}};
inline ContinuousTarget continuousTarget(l01::NavRoute route) noexcept {
    const int p=route.address,q=route.position;
    if(p<0||q<0||p/1024!=0)return {};
    const int t=(p%1024)/32,s=p%32;
    int o=-1,r=-1;
    if(t==0&&s==0&&q<43&&q!=12){o=1328+q;r=o-768;}
    else if(t==1&&s>0&&s<9&&q<16){o=40*(s-1)+q;r=16*(s-1)+q;}
    else if(t==2&&s>0&&s<25&&q<18){o=320+42*(s-1)+q;r=128+18*(s-1)+q;}
    else if(t>2&&t<7&&s>0&&s<9&&q<6){o=1552+48*(t-3)+6*(s-1)+q;r=o-837;}
    else if(t>6&&t<9&&s==0&&q<12){o=1744+12*(t-7)+q;r=o-837;}
    else if(t>8&&t<13){
        if(s==0&&q<3)o=1461+3*(t-9)+q;
        else if(s==0&&q==3)o=1473+t-9;
        else if(s>0&&s<4){
            if(t<12&&q<6)o=1371+18*(t-9)+6*(s-1)+q;
            else if(t==12&&q<12)o=1425+12*(s-1)+q;
        }
        if(o>=0)r=o-768;
    }else if(t==13&&s==0&&q<6){o=1546+q;r=o-837;}
    if(o<0||o>=1768||r<0||r>=931)return {};
    return {o,r};
}
class ContinuousPickup {
public:
    ContinuousPickup() noexcept {physical_.fill(-1);}
    // Caller has checked known LIVE, channel, route and monotonic gesture time.
    int consider(int cc,int api,int previous,int raw,int64_t now) noexcept {
        if(cc<0||cc>=20||api<0||api>=931||previous<0||previous>127||raw<0||raw>127||now<0)return -1;
        auto& a=state_[static_cast<std::size_t>(api)];
        const auto age=a.time<0?now+1:now-a.time;
        if(age<0)return -1;
        const int threshold=a.time<0?4:age<51?12:age<151?6:4;
        const int distance=raw>=previous?raw-previous:previous-raw;
        const bool fresh=physical_[static_cast<std::size_t>(cc)]!=api||a.known!=previous;
        if(fresh){a.wait=true;a.initialDistance=distance;a.direction=0;a.attempts=0;a.previousDirection=0;a.age=0;a.reversals=0;}
        const int direction=a.raw<0?0:raw>a.raw?1:raw<a.raw?-1:0;
        int output=-1;
        if(!a.wait)output=raw;
        else if(distance<=threshold){
            if((fresh&&2*a.initialDistance<3*threshold+1)||threshold<12)output=raw;
            else if(a.direction!=0){if((direction!=0&&direction!=a.direction)||age>a.age+50)output=raw;}
            else if(direction!=0)a.direction=direction;
        }else{
            if(direction!=0&&a.previousDirection!=0&&direction!=a.previousDirection)++a.reversals;
            ++a.attempts;
            if(a.reversals>2){a.attempts=100;a.reversals=0;}
            if(a.attempts>99){const int step=(distance+5)/10;output=previous+(raw<previous?-step:step);}
            if(direction!=0)a.previousDirection=direction;
            a.age=age;
        }
        // Source type2 catch-up does not latch pickup; only direct acquisition does.
        if(output==raw){a.wait=false;a.direction=0;a.attempts=0;}
        a.known=output>=0?output:previous;physical_[static_cast<std::size_t>(cc)]=api;a.raw=raw;a.time=now;
        return output;
    }
    // A recall must disarm only the controls whose actual target changed.
    void rearm(int api) noexcept {
        if(api<0||api>=931)return;
        state_[static_cast<std::size_t>(api)]={};
        for(auto& physical:physical_)if(physical==api)physical=-1;
    }
private:
    struct State {bool wait{true};int known{-2},raw{-1},initialDistance{},direction{},attempts{},previousDirection{},reversals{};int64_t time{-1},age{};};
    std::array<State,931> state_{};
    std::array<int,20> physical_{};
};
}
