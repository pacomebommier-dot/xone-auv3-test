#pragma once
// Native virtual S7 CUE/LOAD 931..946, qualified by real S3 physical BASE
// route kind 6 (CUE root0) or kind 7 (LOAD root1). This is an abstract MIDI
// control API: no synth program, plugin load, PATCH/BASE applied or ACK inferred.
// Uses the host's authenticated BAR; never an independent timer.
#include "../Core/NavS3.hpp"
#include <array>
#include <cstdint>
namespace xone::host {
class CueLoadBar {
public:
    struct Packet {
        bool valid{};
        unsigned group{},index{},generation{};
        int api{-1},value{};
        std::array<std::uint8_t,3> bytes{};
    };
    struct Channel {
        int applied{-1},pending{-1},due{-1};
        bool dueReady{},blocked{};
        unsigned generation{};
    };
    static bool derive(const l01::NavS3& nav,unsigned physical,
                       unsigned& group,unsigned& index) noexcept {
        if(!nav.stable()||nav.hasStage()||physical<4||physical>=20)return false;
        const auto& n=nav.active();
        const int root=n.at(291);
        if((root!=0&&root!=1)||n.at(292)!=0||n.contextMode!=0||n.presetFlag!=0)
            return false;
        const auto r=n.route(static_cast<int>(physical));
        if(r.address<0||r.position<0||r.position>=4)return false;
        const int kind=r.address/1024;
        const int family=(r.address%1024)/32;
        const int selection=r.address%32;
        if(kind!=6+root||family!=3+root||(selection!=4&&selection!=8))return false;
        const int flag=selection==4?299+2*root:300+2*root;
        const int select=selection==4?293+2*root:294+2*root;
        if(n.at(flag)!=1||n.at(select)!=selection)return false;
        group=static_cast<unsigned>(root*2+(selection==8));
        index=static_cast<unsigned>(selection==8?4:0)+static_cast<unsigned>(r.position);
        return true;
    }
    std::array<int,4> desiredSelections()const noexcept {
        std::array<int,4> result{};
        for(unsigned group=0;group<4;++group){const auto& s=channels_[group];result[group]=s.dueReady?s.due:s.applied;}
        return result;
    }
    bool restoreSelections(const std::array<int,4>& choices) noexcept {
        for(unsigned group=0;group<4;++group)if(choices[group]!=-1&&
            (choices[group]<0||choices[group]>7||((group&1)==0?choices[group]>3:choices[group]<4)))return false;
        for(unsigned group=0;group<4;++group){auto& s=channels_[group];
            s.due=choices[group];s.dueReady=s.applied!=choices[group];s.blocked=false;
            if(++s.generation==0)++s.generation;
        }
        return true; // live pending gestures are preserved, never loaded from an image
    }
    bool request(const l01::NavS3& nav,unsigned physical)noexcept {
        unsigned group=0,index=0;
        if(!derive(nav,physical,group,index))return false;
        return requestQualified(group,index);
    }
    bool requestQualified(unsigned group,unsigned index)noexcept {
        if(group>=channels_.size()||index>=8||
           ((group&1)==0?index>=4:index<4))return false;
        auto& s=channels_[group];
        // Capture the owner at the true DOWN. Later NAV edits cannot
        // move the pending action to a different instrument/group.
        if(s.pending==static_cast<int>(index))return true;
        s.pending=static_cast<int>(index);
        return true;
    }
    void onBar() noexcept {commitPending();}
    // Explicit stopped-transport application, not a manufactured BAR event.
    void onStopped() noexcept {commitPending();}
    void commitPending() noexcept {
        for(unsigned g=0;g<channels_.size();++g){
            auto& s=channels_[g];
            if(s.dueReady||s.pending<0)continue;
            if(s.applied==s.pending){s.pending=-1;continue;}
            s.due=s.pending;s.pending=-1;s.dueReady=true;s.blocked=false;
            if(++s.generation==0)++s.generation;
        }
    }
    void beginRetryPass()noexcept {
        for(auto& s:channels_)s.blocked=false;
    }
    Packet next()const noexcept {
        for(unsigned g=0;g<channels_.size();++g){
            const auto& s=channels_[g];
            if(!s.dueReady||s.blocked)continue;
            const bool off=s.applied>=0;
            const int i=off?s.applied:s.due;
            if(i<0||i>=16)return {};
            const int api=(g<2?931:939)+i%8;
            const int value=off?0:127;
            // OutputS4 kind17 uses 0xB0 + MIDI7 + floor(api/128).
            return {true,g,static_cast<unsigned>(i),s.generation,api,value,
                {static_cast<std::uint8_t>(0xB6+api/128),
                 static_cast<std::uint8_t>(api%128),
                 static_cast<std::uint8_t>(value)}};
        }
        return {};
    }
    bool acknowledge(Packet packet,bool callbackSucceeded)noexcept {
        if(!packet.valid||packet.group>=channels_.size())return false;
        auto& s=channels_[packet.group];
        if(!s.dueReady||s.generation!=packet.generation)return false;
        auto expected=next();
        if(!expected.valid||expected.group!=packet.group||expected.generation!=packet.generation||
           expected.api!=packet.api||expected.value!=packet.value)return false;
        if(!callbackSucceeded){s.blocked=true;return true;}
        if(packet.value==0){s.applied=-1;if(s.due<0)s.dueReady=false;return true;} // OFF ACK before ON
        s.applied=s.due;s.due=-1;s.dueReady=false;s.blocked=false;return true;
    }
    void cancelUnstarted(unsigned group) noexcept {if(group<4)channels_[group].pending=-1;}
    void cancelPending(unsigned group) noexcept {
        if(group>=4)return;
        auto& s=channels_[group];s.pending=-1;s.due=-1;s.dueReady=false;s.blocked=false;
        if(++s.generation==0)++s.generation;
    }
    void stop()noexcept {
        // STOP cancels unstarted changes, never invents reset of an applied
        // CUE/LOAD or assumes an external device changed its program.
        for(auto& s:channels_){
            s.pending=-1;s.due=-1;s.dueReady=false;s.blocked=false;
            if(++s.generation==0)++s.generation;
        }
    }
    int sentSelection(unsigned group)const noexcept {
        return group<channels_.size()?channels_[group].applied:-1;
    }
    int pendingSelection(unsigned group)const noexcept {
        return group<channels_.size()?channels_[group].pending:-1;
    }
    unsigned pendingActions()const noexcept {
        unsigned n=0;
        for(const auto& s:channels_)if(s.pending>=0||s.dueReady)++n;
        return n;
    }
private:
    std::array<Channel,4> channels_{};
};
} // namespace xone::host
