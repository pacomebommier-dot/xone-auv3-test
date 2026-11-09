#pragma once
// E3: Xone K2 relative encoder input, not a MIDI output API.
#include <cstdint>
namespace xone::host {
enum class DjTurnKind : uint8_t { None, BrowserTrack, PerformanceLoopLength };
struct DjTurn {
    DjTurnKind kind{DjTurnKind::None};
    int delta{};
    bool valid()const noexcept{return kind!=DjTurnKind::None&&delta!=0;}
};
inline DjTurn decodeDjTurn(unsigned cc,unsigned raw)noexcept{
    if((cc!=20&&cc!=21)||raw==0||raw>127)return {};
    const int delta=raw<64?static_cast<int>(raw):static_cast<int>(raw)-128;
    return {cc==20?DjTurnKind::BrowserTrack:DjTurnKind::PerformanceLoopLength,delta};
}
}
