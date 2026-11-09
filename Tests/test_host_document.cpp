#include "../Shared/XoneHostEngine.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
int main(){
 HostEngine source;assert(!source.captureMemory(-1,0)&&!source.captureMemory(28,0)&&!source.captureMemory(0,4));SessionImage live{};live.schema=1;live.config=1;live.revision=1;live.desired.fill(-2);
 for(unsigned f=0;f<60;++f)live.desired[static_cast<unsigned>(SourcePatchBank::cell(0,f))]=f%30<18?37:0;
 for(int f=0;f<9;++f)live.desired[static_cast<unsigned>(xone::l01::FxS6::addressFor(0,f))]=f<6?51:0;
 live.desired[1328]=64;live.desired[28]=4;
 assert(source.setS8SampleModeBeforeRendering(7,11,4));assert(source.setS8MachineModeBeforeRendering(23,11,3));
 assert(source.requestSessionRestore(live));source.beginRenderBuffer();
 assert(source.saveSourcePatch(0,2));assert(source.captureMemory(0,3));assert(source.captureMemory(0,3));assert(source.capturePreset(2));
 SourcePatchBank::Values base{};for(unsigned i=0;i<60;++i)base[i]=i%30<18?22:0;
 assert(source.configureAlternativeBase(0,3,base,1));assert(source.bindAlternativeSlot(0,1,3));assert(source.configureAlternativeMode(0,3,23,4));
 assert(source.requestMemoryRecall(0,3));assert(source.captureRed());
 source.endRenderBuffer();auto document=source.documentForHost();DocumentBytes bytes{};assert(encodeDocument(document,bytes));
 DocumentImage round{};assert(decodeDocument(bytes.data(),bytes.size(),round));
 assert(round.sample.mode(7,11)==4&&round.machine.mode(23,11)==3&&round.alternative.mode(0,3,23)==4);
 SourcePatchBank::Patch patch{};assert(round.source.read(0,2,patch)&&patch.values[0]==37);
 assert(round.memories[3].valid&&round.memories[3].values[0]==51&&round.presets[2].valid);
 round.current.cueSelections[0]=2;
 HostEngine restored;restored.enableNativeMutePage(true);assert(restored.requestDocumentRestore(round));assert(restored.documentForHost().presets[2].valid);
 restored.beginRenderBuffer();assert(restored.desiredLive(1328).value==64&&restored.observed()==-2);
 unsigned muteCursor=0,muteCount=0;
 while(true){auto p=restored.nextMutePacket(muteCursor);if(!p.valid)break;assert(p.bytes[0]==0x8A&&p.bytes[2]==0);restored.ackMutePacket(p.target,true);++muteCount;}
 assert(muteCount==16); // imported applied OFFs are not silently considered already sent
 assert(!restored.nextCueLoadPacket().valid);restored.endRenderBuffer();assert(restored.documentForHost().current.cueSelections[0]==2);
 assert(restored.readSourcePatch(0,2,patch)&&patch.values[0]==37);
 xone::l01::BankS6::MemoryRecord memory{};assert(restored.readMemory(0,3,memory)&&memory.valid&&memory.values[0]==51&&memory.revision==2);
 assert(restored.captureMemory(0,3)&&restored.readMemory(0,3,memory)&&memory.revision==3);
 assert(restored.pendingMemorySlot(0)==-1&&!restored.redActive());
 assert(restored.s8MachineMode(23,11)==3&&restored.s8SampleMode(7,11)==4);assert(restored.requestPresetRecall(2));
 const auto unchanged=round.live.desired;
 assert(!decodeDocument(bytes.data(),bytes.size()-1,round));assert(round.live.desired==unchanged);
 auto bad=bytes;bad[20]^=1;assert(!decodeDocument(bad.data(),bad.size(),round));bad=bytes;bad[4]=3;assert(!decodeDocument(bad.data(),bad.size(),round));
 auto invalid=round;invalid.memories[3].values[6]=2;assert(!restored.requestDocumentRestore(invalid));
 invalid=round;invalid.current.alternatives[0]=2;assert(!restored.requestDocumentRestore(invalid));
 uint8_t down[]{0x9E,36,127},up[]{0x8E,36,0};restored.receive(0,down,3,1,1);
 auto changed=round;changed.live.desired[1328]=19;changed.current.live.desired[1328]=19;
 assert(restored.requestDocumentRestore(changed));restored.beginRenderBuffer();assert(restored.desiredLive(1328).value==64);
 restored.receive(0,up,3,2,2);restored.beginRenderBuffer();assert(restored.desiredLive(1328).value==19);
 auto packet=restored.nextSourcePacket();assert(packet.valid);restored.ackSourcePacket(packet,false,3);assert(!restored.nextSourcePacket().valid);
 restored.beginSourceRetryPass();assert(restored.nextSourcePacket().valid);
 HostEngine latest;auto v1=live;v1.desired[1328]=7;
 assert(latest.requestDocumentRestore(changed));assert(latest.requestSessionRestore(v1));latest.beginRenderBuffer();assert(latest.desiredLive(1328).value==7);
 assert(latest.requestSessionRestore(v1));assert(latest.requestDocumentRestore(changed));latest.beginRenderBuffer();assert(latest.desiredLive(1328).value==19);
 for(unsigned i=0;i<2048;++i){auto p=latest.nextSourcePacket();if(!p.valid)break;assert(!(p.type==11&&p.api==12));latest.ackSourcePacket(p,true,4);}
 // Fully populated native S8 banks and all snapshots fit the bounded wire image.
 auto full=round;S8SamplePatchBank::Values sv{};S8MachinePatchBank::Values mv{};
 for(unsigned v=0;v<8;++v)for(unsigned slot=0;slot<3;++slot)assert(full.sample.save(v,slot,1,sv,0));
 for(unsigned v=0;v<24;++v)for(unsigned slot=0;slot<3;++slot)assert(full.machine.save(v,slot,1,mv,0));
 for(auto& p:full.presets)p=full.current;
 full.init=full.current;assert(encodeDocument(full,bytes)&&bytes.size()<MaxDocumentBytes);
 assert(decodeDocument(bytes.data(),bytes.size(),round));
 // Decoder rejects an in-range CRC-repaired but invalid scalar (bool=2).
 bytes[8+12+4*xone::l01::LiveState::CellCount]=2;
 sessionPut32(bytes.data()+bytes.size()-4,sessionCrc(bytes.data(),bytes.size()-4));
 assert(!decodeDocument(bytes.data(),bytes.size(),round));
 std::cout<<"PASS document banks/PRESET/configuration roundtrip, CRC/schema/typed rejection, held boundary and nominal retry\n";
}
