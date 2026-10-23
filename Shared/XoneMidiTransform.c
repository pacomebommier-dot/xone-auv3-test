#include "XoneMidiTransform.h"

int XoneTransformMIDI(const uint8_t *input, size_t length, uint8_t output[3]) {
    if (!input || !output || length != 3) return 0;
    if ((input[0] & 0xF0u) != 0xB0u) return 0; // Control Change seulement
    if (input[1] > 127 || input[2] > 127) return 0;

    uint8_t targetCC;
    switch (input[1]) {
        case 4: targetCC = 74; break; // Commande A : CC 4 -> CC 74
        case 8: targetCC = 71; break; // Commande B : CC 8 -> CC 71
        default: return 0;
    }
    output[0] = 0xB0;           // Canal de sortie 1 (à dessein)
    output[1] = targetCC;
    output[2] = input[2];       // Valeur préservée intégralement
    return 1;
}
