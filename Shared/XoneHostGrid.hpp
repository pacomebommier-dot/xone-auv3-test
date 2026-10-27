#pragma once
// L05.1: host-derived musical grid. No self-running metronome and no MIDI I/O.
// Sample offsets are relative to the beginning of this render buffer.
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
namespace xone::host {
struct MusicalContext {
    double tempo{}, numerator{}, beatPosition{}, measureDownbeat{}, sampleRate{};
    int denominator{};
    std::int64_t offsetToNextBeat{};
    bool moving{}, available{};
};
enum class GridKind : unsigned char { Beat, Bar };
struct GridEvent { GridKind kind{}; std::uint32_t offset{}; std::int64_t ordinal{}; };
struct GridBatch {
    std::array<GridEvent,64> events{};
    unsigned count{}; bool overflow{}, valid{};
};
class HostGrid {
public:
    // Do not extrapolate across render calls. The host owns tempo, meter and transport.
    void reset() noexcept { armed_=false; }
    GridBatch scan(const MusicalContext& c, std::uint32_t frames) noexcept {
        GridBatch out{};
        if (!c.available || !c.moving || frames==0 ||
            !std::isfinite(c.tempo) || c.tempo<=0 || c.tempo>1000 ||
            !std::isfinite(c.numerator) || c.numerator<1 || c.numerator>64 ||
            std::floor(c.numerator)!=c.numerator ||
            (c.denominator!=1 && c.denominator!=2 && c.denominator!=4 &&
             c.denominator!=8 && c.denominator!=16 && c.denominator!=32) ||
            !std::isfinite(c.beatPosition) || !std::isfinite(c.measureDownbeat) ||
            c.beatPosition<0 || c.measureDownbeat<0 ||
            !std::isfinite(c.sampleRate) || c.sampleRate<8000 || c.sampleRate>384000 ||
            c.offsetToNextBeat<0) {reset();return out;}
        const double beatsPerBar=c.numerator*4.0/c.denominator;
        const double samplesPerBeat=c.sampleRate*60.0/c.tempo;
        if (!std::isfinite(samplesPerBeat) || samplesPerBeat<1 ||
            c.offsetToNextBeat>static_cast<std::int64_t>(std::ceil(samplesPerBeat))+1) {
            reset();return out;
        }
        // Beat position supplied at buffer start, never inferred from uptime.
        const double beatAtNext=c.beatPosition+c.offsetToNextBeat/samplesPerBeat;
        const double integerBeat=std::round(beatAtNext);
        if(std::abs(beatAtNext-integerBeat)>0.02 || integerBeat<0 ||
           integerBeat>static_cast<double>(std::numeric_limits<std::int64_t>::max()/2)){
            reset();return out;
        }
        out.valid=true;
        for(unsigned i=0;i<64;++i){
            const double at=static_cast<double>(c.offsetToNextBeat)+i*samplesPerBeat;
            if(at>=frames-0.001)break;
            const auto offset=static_cast<std::uint32_t>(std::llround(at));
            if(offset>=frames)break;
            const auto ordinal=static_cast<std::int64_t>(integerBeat)+i;
            // The first observed edge is a synchronization anchor, not a command.
            // Skip an already-seen edge when the host calls twice at the same beat.
            if(armed_ && ordinal<=lastOrdinal_)continue;
            if(!armed_){armed_=true;lastOrdinal_=ordinal;continue;}
            lastOrdinal_=ordinal;
            const double measurePhase=(static_cast<double>(ordinal)-c.measureDownbeat)/beatsPerBar;
            const bool bar=std::abs(measurePhase-std::round(measurePhase))<0.0001;
            if(out.count>=out.events.size()){out.overflow=true;break;}
            out.events[out.count++]={bar?GridKind::Bar:GridKind::Beat,offset,ordinal};
        }
        return out;
    }
private:
    bool armed_{}; std::int64_t lastOrdinal_{-1};
};
} // namespace xone::host
