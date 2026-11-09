#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
using namespace xone::host;
int main(){
 HostEngine e;e.beginRenderBuffer();e.endRenderBuffer();
 const auto count=e.documentPublicationsForDiagnostic();
 assert(count==1);
 const auto start=std::chrono::steady_clock::now();
 for(int i=1;i<=4096;++i){
  e.beginRenderBuffer();GridBatch grid{};grid.valid=true;grid.count=1;grid.events[0]={GridKind::Bar,0,i};e.onMusicalGrid(grid);e.advance(i,i*128);e.prepareMuteLeds(i);e.endRenderBuffer();
 }
 const uint8_t foreign[]{0xB0,74,63};
 for(int i=4097;i<4100;++i){e.receive(1,foreign,3,i*128,i);e.endRenderBuffer();}
 assert(e.documentPublicationsForDiagnostic()==count); // zero full-document copies at rest/untrusted input
 const uint8_t cc[]{0xBE,16,77};
 auto r=e.receive(0,cc,3,600000,5000);assert(r.emit);e.sent(r,0);e.endRenderBuffer();
 assert(e.documentPublicationsForDiagnostic()==count+1);
 assert(e.documentForHost().live.desired[1328]==77);
 // Timer-driven MUTE/NAV still publish even when LIVE revision does not change.
 e.enableNativeMutePage(true);
 const uint8_t down[]{0x9E,13,127},up[]{0x8E,13,0};
 e.receive(0,down,3,600001,5001);e.endRenderBuffer();
 auto before=e.documentPublicationsForDiagnostic();
 e.advance(5201,600128);e.endRenderBuffer();
 assert(e.documentPublicationsForDiagnostic()==before+1);
 assert(e.documentForHost().current.muteVisible);
 e.receive(0,up,3,600129,5202);e.endRenderBuffer();
 assert(!e.documentForHost().current.muteVisible);
 const auto elapsed=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"PASS idle4096 buffers without full-document publication, CC and long MUTE persistence; elapsed_us="<<elapsed<<" document_bytes="<<sizeof(DocumentImage)<<" engine_bytes="<<sizeof(HostEngine)<<"; software measurement only\n";
}
