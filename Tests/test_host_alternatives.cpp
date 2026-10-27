#include "../Shared/XoneAlternativeArbiter.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
using A=AlternativeArbiter;
int main(){
    A a;
    const A::Ready ready{{true,true,true,true}},unknown{{false,true,true,true}};
    assert(a.request(0,1,1)==A::Request::Accepted);
    assert(a.request(1,1,1)==A::Request::Rejected); // C03: first wins same arbitration
    assert(a.pending()[0]==1 && a.pending()[1]==-1);
    assert(a.request(1,1,2)==A::Request::Accepted); // C01: later request steals pending only
    assert(a.pending()[0]==-1 && a.pending()[1]==1);
    auto p=a.prepareNextBar(ready);assert(p.valid&&p.to[1]==1);
    assert(!a.finalize(p,false)&&a.active()[1]<0); // OFF failed: no logical commit
    assert(a.finalize(p,true)&&a.active()[1]==1);
    assert(a.request(0,1,3)==A::Request::Rejected); // ACTIVE may not be stolen
    assert(a.request(0,2,4)==A::Request::Accepted);
    p=a.prepareNextBar(unknown);assert(!p.valid&&a.active()[0]==-1);
    p=a.prepareNextBar(ready);assert(p.valid);
    a.stop();assert(a.pending()[0]==-1 && a.active()[1]==1);
    assert(!a.finalize(p,true)); // stale lease invalid
    assert(a.request(0,2,5)==A::Request::Accepted);
    assert(a.request(0,2,5)==A::Request::SamePending);
    assert(a.request(0,3,6)==A::Request::Accepted);
    p=a.prepareNextBar(ready);assert(p.to[0]==3);
    assert(a.finalize(p,true)&&a.active()[0]==3);
    // The same AUM identifier can exist in a DIFFERENT physical pool.
    assert(a.request(2,1,7)==A::Request::Accepted);
    p=a.prepareNextBar(ready);assert(a.finalize(p,true)&&a.active()[2]==1);
    const auto old=a.active(),rev=a.revision();
    const A::Targets duplicate{{2,2,0,1}};
    p=a.prepareRecall(duplicate,ready);assert(!p.valid); // C04 full rejection
    assert(!a.finalize(p,true)&&a.active()==old&&a.revision()==rev);
    const A::Targets valid{{1,3,2,0}};
    p=a.prepareRecall(valid,unknown);assert(!p.valid);
    p=a.prepareRecall(valid,ready);assert(p.valid&&p.recall);
    assert(a.finalize(p,true)&&a.active()==valid);
    assert(a.request(0,1,8)==A::Request::SamePending);
    assert(a.request(0,0,8)==A::Request::Accepted);
    assert(a.request(0,1,9)==A::Request::Cancelled); // selected current ACTIVE cancels pending
    assert(a.pending()[0]==-1&&a.active()[0]==1);
    // Cancel/STOP does not claim an output event, no fake MIDI or live replay.
    a.stop();assert(a.active()==valid);
    assert(!A::coherent(A::Targets{{1,1,-1,-1}}));
    assert(!A::coherent(A::Targets{{-2,-1,-1,-1}}));
    std::cout<<"PASS C01 C03 C04 active/pending, BAR, cancel/STOP, BASE gate, stale plan, atomic recall\n";
}
