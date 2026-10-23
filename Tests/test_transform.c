#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "XoneMidiTransform.h"

static void check(uint8_t status, uint8_t controller, uint8_t value, int expected,
                  uint8_t expectedCC, uint8_t expectedValue) {
    uint8_t input[3] = {status,controller,value};
    uint8_t output[3] = {0xFF,0xFF,0xFF};
    int done = XoneTransformMIDI(input,3,output);
    assert(done == expected);
    if (done) {
        assert(output[0] == 0xB0);
        assert(output[1] == expectedCC);
        assert(output[2] == expectedValue);
    }
}
int main(void) {
    check(0xB0, 4, 0, 1, 74, 0);
    check(0xB0, 4, 127, 1, 74, 127);
    check(0xBE, 4, 73, 1, 74, 73); // canal entrant 15 -> sortant 1
    check(0xBF, 8, 100, 1, 71, 100);
    check(0xB0, 8, 64, 1, 71, 64);
    check(0xB0, 9, 64, 0, 0, 0);
    check(0x90, 4, 127, 0, 0, 0);
    check(0xB0, 4, 128, 0, 0, 0);
    uint8_t in[2] = {0xB0,4}, out[3] = {0};
    assert(XoneTransformMIDI(in,2,out)==0);
    assert(XoneTransformMIDI(NULL,3,out)==0);
    printf("OK - 10 tests de transformation MIDI\n");
    return 0;
}
