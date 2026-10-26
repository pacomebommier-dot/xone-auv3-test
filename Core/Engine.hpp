#pragma once
#include <array>
#include <cstdint>
namespace xone {
enum class Kind { Down, Up, Navigate, Feedback, Defer, Bar, Stop };
enum class CommandKind { On, Off, Control };
struct Event { std::uint64_t sequence; Kind kind; unsigned char destination, number, value; };
struct Command { CommandKind kind{}; unsigned char destination{}, number{}, value{}; std::uint64_t sequence{}; };
struct KnownValue { bool known{}; unsigned char value{}; };
struct Held { bool pressed{}, sounding{}; unsigned char destination{}; };
struct Pending { bool valid{}; Event event{}; };
struct State {
    unsigned char navigation{};
    std::array<KnownValue,2> observed{};
    std::array<Held,128> held{};
    std::array<Pending,2> pending{};
};
// Research prototype: a single physical input source, two logical destinations.
// Not the complete gesture model, C01/C03/C04 arbiter or AUv3 scheduler.
class Engine {
public:
    static constexpr unsigned OutputCapacity=8;
    bool apply(Event e) noexcept {
        if (e.sequence<=last_ || e.destination>=2 || e.number>127 || e.value>127) return false;
        unsigned needed=0;
        switch(e.kind) {
        case Kind::Down:
            if(state_.held[e.number].pressed) return false;
            for (const auto& h:state_.held) if(h.sounding) ++needed;
            if(needed>=OutputCapacity) return false; // STOP must always fit after drain.
            needed=1; break;
        case Kind::Up:
            if(!state_.held[e.number].pressed) return false;
            needed=state_.held[e.number].sounding ? 1u : 0u; break;
        case Kind::Bar:
            for(const auto& p:state_.pending) if(p.valid) ++needed;
            break;
        case Kind::Stop:
            for(const auto& h:state_.held) if(h.sounding) ++needed;
            break;
        case Kind::Navigate: case Kind::Feedback: case Kind::Defer: break;
        default: return false;
        }
        // Reserve the whole output batch before changing any authoritative field.
        if(needed>OutputCapacity-size_) return false;
        last_=e.sequence;
        switch(e.kind) {
        case Kind::Down: {
            auto& h=state_.held[e.number];
            h={true,true,state_.navigation};
            emit(CommandKind::On,h.destination,e.number,e.value,e.sequence);
            break;
        }
        case Kind::Up: {
            auto& h=state_.held[e.number];
            if(h.sounding) emit(CommandKind::Off,h.destination,e.number,0,e.sequence);
            h={}; break;
        }
        case Kind::Navigate: state_.navigation=e.destination; break;
        case Kind::Feedback:
            // Input is already classified by a trusted test adapter. No echo inference.
            state_.observed[e.destination]={true,e.value}; break;
        case Kind::Defer:
            state_.pending[e.destination]={true,e}; break;
        case Kind::Bar: {
            unsigned first=0;
            if(!state_.pending[0].valid ||
               (state_.pending[1].valid && state_.pending[1].event.sequence<state_.pending[0].event.sequence)) first=1;
            for(unsigned n=0;n<2;++n) {
                auto& p=state_.pending[(first+n)%2];
                if(p.valid) {
                    emit(CommandKind::Control,p.event.destination,p.event.number,p.event.value,e.sequence);
                    p.valid=false;
                }
            }
            break;
        }
        case Kind::Stop:
            for(auto& p:state_.pending) p.valid=false;
            for(unsigned n=0;n<state_.held.size();++n) {
                auto& h=state_.held[n];
                if(h.sounding) {
                    emit(CommandKind::Off,h.destination,static_cast<unsigned char>(n),0,e.sequence);
                    h.sounding=false;
                }
            }
            break;
        }
        return true;
    }
    const State& state() const noexcept { return state_; }
    unsigned count() const noexcept { return size_; }
    const Command& command(unsigned index) const noexcept { return output_[index]; }
    // Consumer must have copied/accepted the complete output batch before drain.
    void drain() noexcept { size_=0; }
private:
    void emit(CommandKind kind,unsigned char destination,unsigned char number,
              unsigned char value,std::uint64_t sequence) noexcept {
        output_[size_++]={kind,destination,number,value,sequence};
    }
    State state_{};
    std::array<Command,OutputCapacity> output_{};
    unsigned size_{};
    std::uint64_t last_{};
};
}
