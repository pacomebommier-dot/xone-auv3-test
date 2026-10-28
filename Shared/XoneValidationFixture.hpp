#pragma once
// Explicit simulated AUM recipe, extracted from test_auv3_document.mm.
// Never used at startup. No instrument profile, BASE, preset bank or observation.
#include "XoneSession.hpp"
namespace xone::host {
inline SessionImage aumValidationFixture() noexcept {
    SessionImage image{};image.schema=1;image.config=1;image.revision=1;
    image.desired.fill(-2);
    image.desired[1328]=64;image.desired[1768]=0;
    image.desired[380]=0;image.desired[392]=3;
    image.desired[381]=0;image.desired[393]=4;
    image.desired[382]=1;image.desired[394]=4;
    return image;
}
}
