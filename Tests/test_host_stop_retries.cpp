#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static SessionImage image(){
    SessionImage i{};i.schema=1;i.config=1;i.revision=1;i.desired.fill(-2);
    for(int c=1477;c<1513;++c)i.desired[c]=0;
    for(int c=1768;c<1864;++c)i.desired[c]=0;
    return i;
}
static Result edge(HostEngine& e,int note,bool down,int ms){
    const uint8_t b[]{static_cast<uint8_t>(down?0x9E:0x8E),
        static_cast<uint8_t>(note),static_cast<uint8_t>(down?127:0)};
    return e.receive(0,b,3,ms,ms);
}
static void select(HostEngine& e){
    assert(!edge(e,36,true,0).emit);
    assert(!edge(e,36,false,1).emit);
}
static void send(HostEngine& e,const Result& r,int status){
    assert(r.emit&&e.canSend(r));
    e.sent(r,status);
}
int main(){
    // Failed callback: the very next buffer retries without another STOP.
    {
        HostEngine e;assert(e.restoreSessionBeforeRendering(image()));select(e);
        auto on=edge(e,48,true,10);send(e,on,0);
        e.stop(20);assert(e.pendingStopOffs()==1);
        auto off=e.nextStopPacket();assert(off.emit&&off.api==384&&off.value==0);
        send(e,off,-1);assert(e.pendingStopOffs()==1);
        assert(!e.nextStopPacket().emit);
        e.beginStopRetryPass();
        off=e.nextStopPacket();assert(off.emit&&off.api==384);
        send(e,off,0);assert(e.pendingStopOffs()==0);
        e.beginStopRetryPass();assert(!e.nextStopPacket().emit);
    }
    // Missing output also retains the OFF; no fabricated acknowledgement.
    {
        HostEngine e;assert(e.restoreSessionBeforeRendering(image()));select(e);
        send(e,edge(e,48,true,10),0);
        e.stop(20);auto off=e.nextStopPacket();assert(off.emit);
        e.noOutput(off);assert(e.pendingStopOffs()==1);
        e.beginStopRetryPass();off=e.nextStopPacket();assert(off.emit);
        send(e,off,0);assert(e.pendingStopOffs()==0);
    }
    // A failure on the middle address must not block peers or resend a
    // confirmed OFF. A second STOP safely resnapshots remaining actual debt.
    {
        HostEngine e;assert(e.restoreSessionBeforeRendering(image()));select(e);
        for(int note:{48,44,40})send(e,edge(e,note,true,10),0);
        e.stop(20);assert(e.pendingStopOffs()==3);
        for(int n=0;n<3;++n){
            auto off=e.nextStopPacket();assert(off.emit&&off.api==384+n);
            send(e,off,n==1?-1:0);
        }
        assert(e.pendingStopOffs()==1);
        e.beginStopRetryPass();
        auto retry=e.nextStopPacket();assert(retry.emit&&retry.api==385);
        assert(!e.nextStopPacket().emit);
        e.stop(30);assert(e.pendingStopOffs()==1);
        retry=e.nextStopPacket();assert(retry.emit&&retry.api==385);
        send(e,retry,0);assert(e.pendingStopOffs()==0);
        e.beginStopRetryPass();assert(!e.nextStopPacket().emit);
    }
    // Successful STOP OFF is permanently retired; no stale OFF may
    // accidentally close the next note on the same address.
    {
        HostEngine e;assert(e.restoreSessionBeforeRendering(image()));select(e);
        send(e,edge(e,48,true,10),0);
        e.stop(20);send(e,e.nextStopPacket(),0);
        assert(!edge(e,48,false,21).emit);
        auto next=edge(e,48,true,30);assert(next.emit&&next.api==384);
        send(e,next,0);e.beginStopRetryPass();
        assert(!e.nextStopPacket().emit&&e.pendingStopOffs()==0);
    }
    std::cout<<"PASS STOP OFF retry on failed/absent callback, independent peers, resnapshot, no stale resend\n";
}
