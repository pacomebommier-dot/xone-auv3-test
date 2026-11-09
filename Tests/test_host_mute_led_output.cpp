#include "../Shared/XoneMuteLedOutput.hpp"
#include <cassert>
#include <vector>
#include <iostream>
using namespace xone::host;
using Packet=MuteLedOutput::Packet;
static std::vector<Packet> flush(MuteLedOutput& out){
 std::vector<Packet> result;unsigned cursor=0;
 for(unsigned n=0;n<64;++n){auto p=out.next(cursor);if(!p.valid)break;result.push_back(p);out.confirm(p,true);}
 return result;
}
int main(){
 GlobalMuteState m;MuteLedOutput out;
 out.observe(m,false,0);assert(flush(out).empty());
 out.observe(m,true,0);auto tx=flush(out);assert(tx.size()==64);
 const int red[]{36,37,38,39,32,33,34,35,28,29,30,31,24,25,26,27};
 for(unsigned i=0;i<16;++i){
  for(unsigned c=0;c<3;++c)assert(tx[i*4+c].bytes[0]==0x8E&&tx[i*4+c].bytes[1]==red[i]+36*c&&tx[i*4+c].bytes[2]==0);
  assert(tx[i*4+3].bytes[0]==0x9E&&tx[i*4+3].bytes[1]==red[i]+72&&tx[i*4+3].bytes[2]==127);
 }
 assert(flush(out).empty());
 assert(m.arm(0,GlobalMuteState::Gesture::ShortMute));
 out.observe(m,true,99);tx=flush(out);assert(tx.size()==2&&tx[0].bytes[1]==108&&tx[0].bytes[0]==0x8E&&tx[1].bytes[1]==36&&tx[1].bytes[0]==0x9E);
 out.observe(m,true,100);tx=flush(out);assert(tx.size()==1&&tx[0].bytes[0]==0x8E&&tx[0].bytes[1]==36);
 out.observe(m,true,200);tx=flush(out);assert(tx.size()==1&&tx[0].bytes[0]==0x9E&&tx[0].bytes[1]==36);
 m.stop();out.observe(m,true,200);tx=flush(out);assert(tx.size()==2&&tx[1].bytes[1]==108);
 assert(m.arm(1,GlobalMuteState::Gesture::LongSolo));assert(m.onBar());
 out.observe(m,true,200);tx=flush(out);assert(tx.size()==16);
 assert(tx[2].bytes[0]==0x9E&&tx[2].bytes[1]==73); // yellow BASE2
 assert(m.arm(2,GlobalMuteState::Gesture::LongSolo));
 out.observe(m,true,200);tx=flush(out);assert(tx.size()==1&&tx[0].bytes[1]==74);
 assert(m.onBar());out.observe(m,true,300);tx=flush(out);assert(tx.size()==1&&tx[0].bytes[1]==73&&tx[0].bytes[0]==0x8E);
 out.observe(m,false,300);tx=flush(out);assert(tx.size()==2&&tx[0].bytes[1]==74&&tx[0].bytes[0]==0x8E&&tx[1].bytes[1]==99);
 out.observe(m,false,400);assert(flush(out).empty());
 // Re-entry purges stale colors; a failed write remains unknown and retries.
 out.observe(m,true,400);unsigned cursor=0;auto p=out.next(cursor);assert(p.valid);
 out.confirm(p,false);auto retry=out.next(cursor);assert(retry.valid&&retry.target==1);
 out.beginBuffer();cursor=0;retry=out.next(cursor);assert(retry.valid&&retry.bytes[1]==p.bytes[1]);
 tx=flush(out);assert(tx.size()==50); // 48 purge OFF plus SOLO and unaffected global FX ON
 assert(flush(out).empty());
 std::cout<<"PASS S7 physical LED bytes, row-major BASE, blink 100ms, SOLO, STOP, page and retry\n";
}