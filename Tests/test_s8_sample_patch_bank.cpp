#include "../Shared/XoneS8SamplePatchBank.hpp"
#include <cassert>
#include <iostream>
using xone::host::S8SamplePatchBank;
int main(){
    S8SamplePatchBank b;
    S8SamplePatchBank::Patch p{};
    for(unsigned voice=0;voice<8;++voice){
        for(unsigned slot=0;slot<3;++slot){
            assert(b.read(voice,slot,p)&&!p.valid);
        }
        for(unsigned mode=0;mode<12;++mode)
            assert(b.mode(voice,mode)==-2); // no invented instrument mode
    }
    S8SamplePatchBank::Values image{};image.fill(63);
    for(unsigned i=16;i<28;++i)image[i]=i%2;
    assert(S8SamplePatchBank::validate(image));
    assert(b.setMode(0,0,4)&&b.setMode(0,11,2));
    assert(b.save(0,0,3,image,0));
    assert(b.read(0,0,p)&&p.valid&&p.config==3&&p.revision==1);
    assert(b.mode(0,0)==4&&b.mode(0,11)==2);
    assert(!b.save(0,0,3,image,0)); // stale revision
    assert(!b.save(0,0,4,image,1)); // incompatible profile
    assert(!b.save(0,3,3,image,0)&&!b.save(8,0,3,image,0));
    assert(!b.setMode(0,12,1)&&!b.setMode(0,0,0)&&!b.setMode(0,0,5));
    S8SamplePatchBank::Values invalid=image;
    invalid[4]=-2;assert(!b.save(0,1,3,invalid,0));
    invalid=image;invalid[16]=2;assert(!b.save(0,1,3,invalid,0));
    // Changing modes does not rewrite PATCH; another PATCH does not change modes.
    assert(b.setMode(0,0,1));
    assert(b.read(0,0,p)&&p.values==image&&p.revision==1);
    image[0]=99;
    assert(b.save(0,0,3,image,1));
    assert(b.mode(0,0)==1&&b.mode(0,11)==2);
    assert(b.read(0,0,p)&&p.values[0]==99&&p.revision==2);
    // Each of 8 SAMPLE owners has independent empty three-slot bank.
    for(unsigned v=1;v<8;++v)for(unsigned s=0;s<3;++s){
        assert(b.read(v,s,p)&&!p.valid);
        assert(b.save(v,s,3,image,0));
        assert(b.read(v,s,p)&&p.valid&&p.revision==1);
    }
    std::cout<<"PASS S8 SAMPLE 8x3 empty PATCH28, revisions, UNKNOWN, separate SPOT modes, no hidden defaults\n";
}
