#include "../Shared/XoneHostEngine.hpp"
#include "../Shared/XoneSession.hpp"
#include <cassert>
#include <thread>
#include <atomic>
#include <iostream>
using namespace xone::host;
static SessionImage image(int value=64) {
    SessionImage s{};s.schema=1;s.config=1;s.revision=1;s.desired.fill(-2);s.desired[1328]=static_cast<int16_t>(value);return s;
}
int main() {
    SessionBytes bytes{};auto s=image();
    assert(encodeSession(s,bytes));
    SessionImage round{};assert(decodeSession(bytes.data(),bytes.size(),round));
    assert(round.desired==s.desired && round.config==1 && round.revision==1);
    bytes[16+2*1328]^=1;assert(!decodeSession(bytes.data(),bytes.size(),round)); // even in-range corruption
    assert(!decodeSession(bytes.data(),bytes.size()-1,round));
    s.config=0;assert(!encodeSession(s,bytes));
    s=image();s.desired[1]=-1;assert(!encodeSession(s,bytes));
    HostEngine e;assert(e.requestSessionRestore(image()));
    assert(e.sessionForHost().desired[1328]==64); // set/get before first render
    e.beginRenderBuffer();
    assert(e.desiredLive(1328).value==64 && e.observed()==-2);
    uint8_t cc[]{0xBE,16,64};auto r=e.receive(0,cc,3,0,0);
    assert(r.emit && r.bytes[0]==0xBA && r.bytes[1]==48);e.sent(r,0);
    cc[2]=100;r=e.receive(0,cc,3,1,1);assert(r.emit);e.sent(r,0);
    e.endRenderBuffer();assert(e.sessionForHost().desired[1328]==100);
    assert(e.requestSessionRestore(image(32)));e.beginRenderBuffer();
    assert(e.desiredLive(1328).value==32 && e.observed()==-2);
    // Restore while a physical key is held waits for real UP; it never fabricates one.
    uint8_t d[]{0x9E,36,127},u[]{0x8E,36,0};
    e.receive(0,d,3,2,2);assert(e.requestSessionRestore(image(16)));
    e.beginRenderBuffer();assert(e.surfaceHeld(4) && e.desiredLive(1328).value==32);
    e.receive(0,u,3,3,3);e.beginRenderBuffer();assert(e.desiredLive(1328).value==16);
    SessionMailbox mailbox;std::atomic<bool> done{false};
    std::thread writer([&] {
        for(uint32_t i=1;i<=30000;++i) {
            SessionFrame f{};f.token=i;f.image=image(static_cast<int>(i%128));f.image.revision=i;
            f.image.desired.fill(static_cast<int16_t>(i%128));mailbox.publish(f);
        }done.store(true,std::memory_order_release);
    });
    uint32_t last=0;SessionFrame f{};
    do {if(mailbox.take(f)){assert(f.token>=last);last=f.token;
        for(auto v:f.image.desired)assert(v==static_cast<int>(f.token%128));
        assert(f.image.revision==f.token);
    }}while(!done.load(std::memory_order_acquire));
    writer.join();if(mailbox.take(f))last=f.token;
    assert(last==30000);
    std::cout<<"PASS session codec CRC/bounds, coherent SPSC snapshots, host LIVE round-trip, held restore, UNKNOWN\n";
}
