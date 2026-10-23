#ifndef XONE_MIDI_TRANSFORM_H
#define XONE_MIDI_TRANSFORM_H

#include <stdint.h>
#include <stddef.h>

// Retourne 1 uniquement pour un message CC MIDI 1.0 de trois octets reconnu.
// Le résultat (3 octets) est écrit dans output si et seulement si return == 1.
// Les messages des autres contrôleurs ne sont pas retransmis dans cette V0.1.
int XoneTransformMIDI(const uint8_t *input, size_t length, uint8_t output[3]);

#endif
