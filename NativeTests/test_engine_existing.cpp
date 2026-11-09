#include "../Core/Engine.hpp"
#include <cassert>
#include <iostream>
#undef assert
#define assert(condition) do { if (!(condition)) { std::cerr << "FAIL line " << __LINE__ << ": " << #condition << std::endl; return 1; } } while(false)
using namespace xone;
int main() {
    // Acceptance: release follows the original destination despite navigation.
    Engine a;
    assert(a.apply({1, Kind::Down, 0, 60, 100}));
    assert(a.apply({2, Kind::Navigate, 1, 0, 0}));
    assert(a.apply({3, Kind::Up, 0, 60, 0}));
    assert(a.count() == 2);
    assert(a.command(0).destination == 0 && a.command(1).destination == 0);
    assert(a.command(1).kind == CommandKind::Off);
    // TX does not establish a physical observation.
    assert(!a.state().observed[0].known);
    assert(a.apply({4, Kind::Feedback, 0, 0, 73}));
    assert(a.state().observed[0].known && a.state().observed[0].value == 73);
    assert(!a.state().observed[1].known);
    // Two independent requests commit at BAR, in accepted order.
    Engine b;
    assert(b.apply({1, Kind::Defer, 1, 74, 40}));
    assert(b.apply({2, Kind::Defer, 0, 71, 90}));
    assert(b.count() == 0);
    assert(b.apply({3, Kind::Bar, 0, 0, 0}));
    assert(b.count() == 2);
    assert(b.command(0).destination == 1 && b.command(1).destination == 0);
    // STOP cancels pending and serves debt, preserving true UP bookkeeping.
    Engine c;
    assert(c.apply({1, Kind::Down, 0, 60, 127}));
    assert(c.apply({2, Kind::Defer, 1, 74, 30}));
    assert(c.apply({3, Kind::Stop, 0, 0, 0}));
    assert(c.count() == 2 && c.command(1).kind == CommandKind::Off);
    assert(c.apply({4, Kind::Bar, 0, 0, 0}));
    assert(c.count() == 2);
    assert(c.apply({5, Kind::Up, 0, 60, 0}) && c.count() == 2);
    // Rejected ordering, invalid MIDI values and orphan releases mutate nothing.
    assert(!c.apply({5, Kind::Navigate, 1, 0, 0}));
    assert(!c.apply({6, Kind::Feedback, 2, 0, 0}));
    assert(!c.apply({7, Kind::Down, 0, 128, 1}));
    assert(!c.apply({8, Kind::Up, 0, 61, 0}));
    assert(c.count() == 2 && c.state().navigation == 0);
    // Capacity refusal does not lose the note-off debt; retry after drain.
    Engine d;
    for (unsigned i=1;i<=Engine::OutputCapacity;++i) {
        assert(d.apply({i, Kind::Down, 0, static_cast<unsigned char>(i), 1}));
    }
    assert(!d.apply({100, Kind::Stop, 0, 0, 0}));
    assert(d.state().held[1].sounding);
    d.drain();
    assert(d.apply({101, Kind::Stop, 0, 0, 0}));
    assert(d.count() == Engine::OutputCapacity);
    assert(!d.state().held[1].sounding);
    // Same sequence produces identical commands (not object-padding memcmp).
    Engine e, f;
    const Event events[]={{1,Kind::Defer,1,74,50},{2,Kind::Bar,0,0,0}};
    for (const auto& event:events) { assert(e.apply(event)); assert(f.apply(event)); }
    assert(e.count()==f.count());
    for (unsigned i=0;i<e.count();++i) {
        assert(e.command(i).destination==f.command(i).destination);
        assert(e.command(i).number==f.command(i).number);
        assert(e.command(i).value==f.command(i).value);
        assert(e.command(i).sequence==f.command(i).sequence);
    }
    // More held notes than a whole STOP batch can release must be refused.
    Engine g;
    for (unsigned i=1;i<=Engine::OutputCapacity;++i) {
        assert(g.apply({i,Kind::Down,0,static_cast<unsigned char>(i),1}));
        g.drain();
    }
    assert(!g.apply({100,Kind::Down,0,90,1}));
    assert(g.apply({101,Kind::Stop,0,0,0}));
    assert(g.count()==Engine::OutputCapacity);
    std::cout << "Engine bytes=" << sizeof(Engine) << std::endl;
    std::cout << "PASS structural acceptance: origin, observation, BAR, STOP, bounds, replay\n";
}
