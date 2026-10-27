#include "../Shared/XoneHostGrid.hpp"
#include <cassert>
#include <iostream>
using namespace xone::host;
static MusicalContext context(double beat,std::int64_t offset=0) {
  return {120,4,beat,0,48000,4,offset,true,true};
}
int main(){
  HostGrid g;
  auto b=g.scan(context(0),48000);assert(b.valid && b.count==1);
  assert(b.events[0].kind==GridKind::Beat && b.events[0].ordinal==1 && b.events[0].offset==24000);
  b=g.scan(context(2),48000);assert(b.count==2 && b.events[0].ordinal==2 && b.events[1].ordinal==3);
  b=g.scan(context(4),48000);assert(b.count==2 && b.events[0].kind==GridKind::Bar && b.events[0].offset==0);
  b=g.scan(context(4),48000);assert(b.count==0); // duplicate render
  auto stop=context(6);stop.moving=false;assert(!g.scan(stop,480).valid);
  b=g.scan(context(8),24000);assert(b.valid&&b.count==0); // re-arm, no retroactive BAR
  b=g.scan(context(9),48000);assert(b.count==2 && b.events[0].ordinal==9);
  auto bad=context(11);bad.available=false;assert(!g.scan(bad,48000).valid);
  bad=context(12);bad.numerator=3.5;assert(!g.scan(bad,48000).valid);
  bad=context(12);bad.offsetToNextBeat=-1;assert(!g.scan(bad,48000).valid);
  bad=context(12);bad.tempo=0;assert(!g.scan(bad,48000).valid);
  // At 4/4, the measure boundary is a BAR; with 3/4 host downbeat is the authority.
  g.reset();auto three=context(0);three.numerator=3;
  assert(g.scan(three,24000).count==0);
  three.beatPosition=1;assert(g.scan(three,48000).count==2);
  three.beatPosition=3;
  b=g.scan(three,24000);assert(b.count==1&&b.events[0].kind==GridKind::Bar);
  // Backward seek and meter switch cannot replay old scheduled boundaries.
  g.reset();assert(g.scan(context(8),24000).count==0);
  assert(g.scan(context(9),24000).count==1);
  assert(g.scan(context(2),24000).count==0); // seek: re-arm
  assert(g.scan(context(3),24000).count==1);
  auto meter=context(4);meter.numerator=3;
  assert(g.scan(meter,24000).count==0); // meter changed: re-arm
  meter.beatPosition=6;
  assert(g.scan(meter,24000).count==1);
  std::cout<<"PASS host grid boundaries, meter, seek, STOP, missing context, duplicate and rearm\n";
}
