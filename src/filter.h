#pragma once

#define PK_EQ       5
#define LOW_SHELF   6
#define HIGH_SHELF  7

namespace BlackaddrAudio_CabIrLoader {

typedef struct eq_t {
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;

    float y1;
    float y2;
    float x1;
    float x2;

    //Current settings
    int type;
    float fs;
    float f0;
    float Q;
    float G;

    //helper variables
    float c, s, alpha, w0, A;

} eq_coeffs;

typedef struct equalizer_t
{
    size_t nbands;
    eq_coeffs** band;
} eq_filters;

}
