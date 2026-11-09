#pragma once
// L01/06 — native deterministic S3 NAV image port from frozen L10 S3
// @z6 @z7 @z8 @zd @ze @zf @zg @zm @zn @zo @wt; see docs/L01_06_REPORT.md.
// This module does not operate LIVE/FX/PRESET/BANK, MIDI routing or AUv3.
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
namespace xone::l01 {
struct NavRoute { int address{-2}; int position{-2};
    bool operator==(const NavRoute &o) const noexcept { return address==o.address && position==o.position; }
};
struct NavImage {
    std::array<int,133> cells{}; // SOURCE d[291..423]; [0..28] header, [29..132] 52 route pairs
    int contextMode{};          // SOURCE i[4], not part of 133 cells
    int presetFlag{};           // SOURCE i[5], not part of 133 cells
    int sampleSlot{};           // SOURCE b6[0], initialized 0 by S3 @OnLoad, changed @st; not NAV cell
    int at(int sourceIndex) const noexcept {return (sourceIndex>=291 && sourceIndex<=423)?cells[static_cast<std::size_t>(sourceIndex-291)]:-2;}
    int& at(int sourceIndex) noexcept {return cells[static_cast<std::size_t>(sourceIndex-291)];} // trusted internal indices only
    NavRoute route(int physicalJ) const noexcept {
        if(physicalJ<4 || physicalJ>=56)return {};
        const int b=29+2*(physicalJ-4);
        return {cells[static_cast<std::size_t>(b)],cells[static_cast<std::size_t>(b+1)]};
    }
    bool operator==(const NavImage& v) const noexcept {return cells==v.cells&&contextMode==v.contextMode&&presetFlag==v.presetFlag&&sampleSlot==v.sampleSlot;}
};
class NavS3 {
public:
    // SOURCE @z6 default. Commit only after rebuilding 52 routes and validating.
    bool initializeDefault() noexcept {
        NavImage next{};
        next.at(313)=1;next.at(314)=2;
        next.at(317)=1;next.at(318)=2;next.at(319)=3;
        if (!regenerate(next)||!validate(next))return false;
        active_=next;undo_=next;stable_=true;++revision_;return true;
    }
    // SOURCE @wt staging, @ze regenerated mapping; no mutation if the proposal is not valid.
    bool stage(const NavImage& candidate) noexcept {
        if(!stable_)return false;
        staged_=candidate;hasStage_=true;return true;
    }
    bool stageHeader(const std::array<int,29>& header,int contextMode,int presetFlag) noexcept {
        if(!stable_)return false;
        NavImage next=active_;
        for(std::size_t i=0;i<29;++i)next.cells[i]=header[i];
        next.contextMode=contextMode;next.presetFlag=presetFlag;
        if(!regenerate(next))return false;
        return stage(next);
    }
    bool commit() noexcept {
        if(!stable_||!hasStage_||!validate(staged_))return false;
        undo_=active_;active_=staged_;hasStage_=false;++revision_;
        return true;
    }
    void abandon() noexcept {hasStage_=false;}
    bool stable() const noexcept {return stable_;}
    bool hasStage() const noexcept {return hasStage_;}
    const NavImage& active() const noexcept{return active_;}
    const NavImage& undoSnapshot() const noexcept{return undo_;}
    const NavImage& stageSnapshot() const noexcept{return staged_;}
    std::uint64_t revision() const noexcept{return revision_;}
    // SOURCE @zj r2=1 : save active and previous undo (@ww), then enter
    // the qualified mode temporarily.  @z9/@wv/@zi restore both snapshots.
    bool applyLongBeginMode(int h7) noexcept {
        if(!stable_ || hasStage_ || h7<0 || h7>8)return false;
        NavImage candidate=active_;
        if(h7==8){candidate.at(291)=1-candidate.at(291);candidate.at(292)=0;candidate.contextMode=0;candidate.presetFlag=0;}
        else if(h7==0){candidate.at(291)=0;candidate.at(292)=0;candidate.contextMode=0;candidate.presetFlag=0;}
        else if(h7==candidate.at(292))candidate=undo_; // literal @zj LONG current mode -> e[424]
        else {
            candidate.at(292)=h7;
            if(h7==1)candidate.presetFlag=1;
            else if(h7!=5 && h7!=6){candidate.contextMode=h7;candidate.presetFlag=0;}
        }
        if(!regenerate(candidate) || !validate(candidate))return false;
        staged_=candidate;hasStage_=true;
        return commit();
    }
    bool restoreLong(const NavImage& original,const NavImage& previousUndo) noexcept {
        // Restore via validated NAV transaction, with history @zi repaired atomically.
        if(!stable_ || hasStage_ || !validate(original) || !validate(previousUndo))return false;
        undo_=active_;
        active_=original;
        undo_=previousUndo;
        ++revision_;
        return true;
    }
    // SOURCE @za: BOT4 trueUP in MACHINE mode (7), D315/F5/F6 bank cycle.
    bool cycleMachineBank(int& savedLow,int& savedHigh) noexcept {
        if(!stable_||hasStage_||active_.at(292)!=7)return false;
        NavImage next=active_;
        int low=savedLow,high=savedHigh;
        next.at(315)=1-next.at(315);
        if(next.at(315)==1 && next.at(314)>8 && next.at(314)<17){
            low=next.at(314);next.at(314)=17;
            if(high>16&&high<25)next.at(314)=high;
        }else if(next.at(315)==0&&next.at(314)>16){
            high=next.at(314);next.at(314)=2;
            if(low>8&&low<17)next.at(314)=low;
        }
        if(!regenerate(next)||!validate(next))return false;
        staged_=next;hasStage_=true;
        if(!commit())return false;
        savedLow=low;savedHigh=high;return true;
    }
    // SOURCE @zj subset: short (r2=0), no pending browser/long gesture.
    // Known mode intent 0..8 only; no PRESET recall or history restore.
    bool applyShortMode(int h7) noexcept {
        if(!stable_||h7<0||h7>8||hasStage_)return false;
        NavImage candidate=active_;
        if(h7==8){candidate.at(291)=1-candidate.at(291);candidate.at(292)=0;candidate.contextMode=0;candidate.presetFlag=0;}
        else if(h7==0){candidate.at(291)=0;candidate.at(292)=0;candidate.contextMode=0;candidate.presetFlag=0;}
        else if(h7==candidate.at(292)){candidate.at(292)=0;candidate.contextMode=0;candidate.presetFlag=0;}
        else {
            candidate.at(292)=h7;
            if(h7==1)candidate.presetFlag=1;
            else if(h7!=5&&h7!=6){candidate.contextMode=h7;candidate.presetFlag=0;}
        }
        if(!regenerate(candidate)||!validate(candidate))return false;
        staged_=candidate;hasStage_=true;
        return commit();
    }
    static int shortRoleMode(int role) noexcept { // source @si decoded; role0 RED, role10 DJ2 are not a NAV SHORT
        switch(role){case 1:return 1;case 2:return 2;case 3:return 8;case 4:return 3;case 5:return 4;case 6:return 7;case 7:return 5;case 8:return 6;case 9:return 0;default:return -1;}
    }
    // @ze, @zf; route recomputation is deterministic and side effect free.
    static bool regenerate(NavImage& n) noexcept {
        if(n.at(291)<0||n.at(291)>1||n.contextMode<0||n.contextMode>7||n.presetFlag<0||n.presetFlag>1||n.sampleSlot<0||n.sampleSlot>3)return false;
        for(int j=4;j<56;++j){auto p=routeFor(n,j,0);n.at(320+2*(j-4))=p.address;n.at(321+2*(j-4))=p.position;}
        return true;
    }
    static bool validate(const NavImage& n) noexcept { // literal guards @zg, including 52 pair equality tests
        const int p=n.at(291), mode=n.at(292), cm=n.contextMode, pf=n.presetFlag;
        if(p<0||p>1||mode<0||mode>7||(cm!=0&&cm!=2&&cm!=3&&cm!=4&&cm!=7)||(pf!=0&&pf!=1)||n.sampleSlot<0||n.sampleSlot>3)return false;
        if(mode==0){if(cm!=0||pf!=0)return false;}
        else if(mode==1){if(pf!=1)return false;}
        else if(mode!=5&&mode!=6){if(cm!=mode||pf!=0)return false;}
        for(int b=0;b<2;++b){
            const int x=293+2*b,z=297+b,flag=299+2*b,k=303+b;
            if(n.at(x)<0||n.at(x)>4)return false;
            if(n.at(x+1)!=0 && (n.at(x+1)<5||n.at(x+1)>8))return false;
            if(n.at(z)<0||n.at(z)>8)return false;
            for(int i: {flag,flag+1,k})if(n.at(i)<0||n.at(i)>1)return false;
            if(n.at(flag)+n.at(flag+1)+n.at(k)>1)return false;
            if(n.at(flag)==1&&n.at(x)<1)return false;
            if(n.at(flag+1)==1&&n.at(x+1)<5)return false;
            if(n.at(k)==1&&n.at(z)<1)return false;
        }
        for(int i=305;i<309;++i){
            if(n.at(i)<0||n.at(i)>3||n.at(i+4)<0||n.at(i+4)>4)return false;
            if(n.at(i+4)>0 && n.at(i)==0)return false;
            for(int j=i+1;j<309;++j)if(n.at(i+4)>0&&n.at(i+4)==n.at(j+4))return false;
        }
        if(n.at(313)<1||n.at(313)>8||n.at(314)<1||n.at(314)>24||n.at(315)<0||n.at(315)>1)return false;
        for(int i=316;i<320;++i)if(n.at(i)!=i-316)return false;
        for(int j=4;j<56;++j){
            const NavRoute expected=routeFor(n,j,0),actual=n.route(j);
            if(actual.address==-2||!(expected==actual))return false;
        }
        return true;
    }
private:
    static NavRoute routeFor(const NavImage& d,int j,int depth) noexcept {
        if(j<4||j>55||depth>1)return {-2,-2};
        const int page=d.at(291);
        int x=0,z=0;
        const int p5=page==0?299:page==1?301:-1;
        if(p5>=0){
            if(d.at(p5)==1){x=3+page;z=d.at(293+2*page);}
            else if(d.at(p5+1)==1){x=3+page;z=d.at(294+2*page);}
            else if(d.at(303+page)==1){x=5+page;z=d.at(297+page);}
        }
        const int d9=j<20?j-4:j<32?j-20:j>=40&&j<52?j-40:0;
        const int d8=d9/4;
        int k=-1,n0=-1,a0=0,h=-1;
        if(j<20){
            h=4*(d9%4)+d8+1;k=2;n0=3+page;a0=h;
            if(h>8){n0+=2;a0=h-8;}
            h=0;
        }else if(j<32){
            h=3*(d9%4)+d8;
            if(h<6){k=9;n0=7+page;}
            else {k=24;a0=0;h-=6;n0=14+page;}
        }else if(j<40){
            k=j<36?1:0;
            if(j%4==3){n0=12;h=3;}
            else {n0=13;h=j%4;if(h==2)h=4;if(page==1){h+=2;if(h==6)h=5;}}
        }else if(j<52){
            h=3*(d9%4)+d8;k=0;n0=7+page;
        }else{
            k=0;n0=0;h=j-52+2*page;
            if(j==54)h=4+page;
            else if(j==55)h=8+5*page;
        }
        NavRoute r{k*1024+n0*32+a0,h};
        if(x!=0){
            if(j<20){
                h=4*(d9%4)+d8+1;
                if(x==3+page){
                    n0=3+page;a0=h;k=2;
                    if(h>8){n0=5+page;a0=h-8;h=0;}
                    else if((z<5&&h>4)||(z>4&&h<5)){
                        k=5;if(z==4||z==8)k=6+page;a0=z;
                        if(z<5)h-=5;else h-=1;
                    }else h=0;
                }else if(x==5+page){
                    if(h<5){k=8;n0=5+page;a0=z;h-=1;}
                    else if(h<8){k=5;n0=5+page;a0=z;h-=5;}
                    else if(h==8){r={-1,-1};}
                    else{k=2;n0=5+page;a0=h-8;h=0;}
                }
            } else if(j<32){
                h=3*(d9%4)+d8;n0=x;a0=z;
                if(x==3+page){
                    if(h<3)k=3;
                    else if(h<6)k=1;
                    else {k=24;n0=14+page;a0=0;h-=6;}
                }else if(x==5+page){
                    if(h<6){k=9;n0=7+page;a0=0;}
                    else if(h<9){k=3;h-=6;}
                    else{k=1;h-=6;}
                }
            } else if(j>=40&&j<52){
                k=0;
                if(x==3+page){
                    if(h<6){n0=3+page;a0=z;}
                    else{n0=7+page;a0=0;}
                }else if(x==5+page){
                    if(h<6){n0=7+page;a0=0;}
                    else{n0=5+page;a0=z;h-=6;}
                }
            }
            if(r.address!=-1)r={k*1024+n0*32+a0,h};
        }
        // SOURCE @zm overlays; port separately from the mapping core.
        if(d.contextMode==3||d.contextMode==7){
            n0=d.contextMode==3?1:2;a0=d.at(d.contextMode==3?313:314);k=0;h=0;
            if(j<20){
                h=4*(d9%4)+d8+1;
                if(n0==1){if(h>8)k=-1;else{k=2;a0=h;h=0;}}
                else{if(d.at(315)==1){if(h<9)k=-1;else a0=h+8;}else a0=h;
                    if(k==0){k=2;h=0;}}
            }else if(j<32){k=3;h=3*(d9%4)+d8;}
            else if(j<36){if(j==35){k=1;n0=12;a0=0;h=3;}else{k=11;h=j-32;}}
            else if(j<40){if(j==39){n0=12;a0=0;h=3;}
                else if(n0==1)h=j-23;else h=j-21;}
            else if(j<52){h=3*(d9%4)+d8;}
            else if(j==55){n0=0;a0=0;
                if(d.contextMode==3)h=22;
                else if(d.at(314)<9)h=23+(d.at(314)-1)/2;
                else h=d.at(314)+18;
            }else if(n0==1){if(j==52)h=12;else{n0=0;a0=0;h=j-47;}}
            else h=j-40;
            r=k==-1?NavRoute{-1,-1}:NavRoute{k*1024+n0*32+a0,h};
        } else if(d.contextMode==2||d.contextMode==4){
            // SOURCE @zo including priority @d[309..312].
            const bool highest=d.at(312)>0 && [&](){for(int ii=309;ii<312;++ii)if(d.at(ii)>0&&d.at(ii)<d.at(312))return false;return true;}();
            k=0;n0=12;a0=0;h=3;
            if(j<20){
                const int f1=d9%4,q0=d9/4;n0=9+f1;
                if(q0==3){k=15;a0=0;h=0;}
                else{k=2;a0=q0+1;h=0;}
            }else if(j<32||(j>39&&j<52)){
                const int f1=d9%4,q0=d9/4;
                int f9=f1;h=q0;
                if(highest){f9=3;a0=d.at(308);h=4*q0;if(f1!=3)h+=f1+1;}
                else{
                    if(f1<2){
                        if(d.at(309)>0){if(d.at(310)==0||d.at(309)<d.at(310))f9=0;else f9=1;}
                        else if(d.at(310)>0)f9=1;
                    }else if(d.at(311)>0)f9=2;
                    if(d.at(309+f9)>0){a0=d.at(305+f9);h=2*q0;if(f9!=f1)++h;}
                    else{a0=0;f9=f1;}
                }
                n0=9+f9;
                if(j<32){if(a0>0&&f9==f1){k=3;h=q0;}else k=1;}
            }else if(j<40){
                if(j<36){const int f1=j-32;k=1;n0=9+f1;}
                else n0=9+(j-36);
            }else{n0=0;h=j-38;if(d.contextMode==4){h=j-43;if(j==55)h=13;}}
            r={k*1024+n0*32+a0,h};
        }
        if(d.presetFlag==1&&j<20)r={14*1024+17*32,j-4};
        if(d.at(292)==6&&j<20){
            if(j<12){r={25*1024,j-4};if(j>7)r={26*1024+32*d.sampleSlot,j-8};} // @zm reads b6[0], explicitly captured
            else if(j>15)r={12*1024,j-16};
            else if(j==12)r={23*1024+16*32,4};
            else r={-1,-1};
        }else if(d.at(292)==5){
            if(j<20&&d.presetFlag==0){
                if(d.contextMode==0){a0=4*(d9%4)+d8+1;n0=3+page;
                    if(a0>8){a0-=8;n0=5+page;}
                    r={2*1024+32*n0+a0,0};}
                if(r.address>-1){k=r.address/1024;
                    if(k==2)r.address+=11*1024;
                    else if(k!=15)r={-1,-1};}
            }else if(j>19&&j<32){
                // @zm recursive @zf(j+20), at most one level, with j restored afterwards.
                if(depth>=1)return {-2,-2};
                r=routeFor(d,j+20,depth+1);
                if(r.address>-1)r.address+=1024;
            }
        }
        return r;
    }
    NavImage active_{};NavImage staged_{};NavImage undo_{};
    bool stable_{},hasStage_{};std::uint64_t revision_{};
};
}
