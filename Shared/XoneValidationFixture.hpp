#pragma once
// Explicit simulated AUM recipe, extracted from test_auv3_document.mm.
// Never used at startup. No instrument profile, BASE, preset bank or observation.
#include "XoneSession.hpp"
namespace xone::host {
inline SessionImage aumValidationFixture() noexcept {
    SessionImage image{};image.schema=1;image.config=1;image.revision=1;
    image.desired.fill(-2);
    image.desired[1328]=64;image.desired[1768]=0;
    image.desired[338]=0;image.desired[350]=3;
    image.desired[339]=0;image.desired[351]=4;
    image.desired[340]=1;image.desired[352]=4;
    return image;
}
}
