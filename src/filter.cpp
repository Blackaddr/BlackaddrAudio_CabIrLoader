#include "filter.h"

namespace BlackaddrAudio_CabIrLoader {

void
eq_compute_coeffs(eq_coeffs* cf, int type, float fs, float f0, float Q, float G)
{
    float A = powf(10.0, G/40.0);
    float w0 = 2.0*M_PI*f0/fs;
    float c = cosf(w0);
    float s = sinf(w0);

    float alpha = s/(2.0*Q);
    float a0, a1, a2;
    float b0, b1, b2;
    a0 = a1 = a2 = 0.0;
    b0 = b1 = b2 = 0.0;

    cf->fs = fs;
    cf->f0 = f0;
    cf->Q = Q;
    cf->G = G;
    cf->type = type;

    cf->A = A;
    cf->w0 = w0;
    cf->c = c;
    cf->s = s;
    cf->alpha = alpha;

    switch(type){

            case PK_EQ:
                b0 =   1.0 + alpha*A;
                b1 =  -2.0*c;
                b2 =   1.0 - alpha*A;
                a0 =   1.0 + alpha/A;
                a1 =  -2.0*c;
                a2 =   1.0 - alpha/A;
                break;

            case LOW_SHELF:
                b0 =  A*( (A+1.0) - (A-1.0)*c + 2.0*sqrtf(A)*alpha);
                b1 =  2.0*A*( (A-1.0) - (A+1.0)*c);
                b2 =  A*( (A+1.0) - (A-1.0)*c - 2.0*sqrtf(A)*alpha);
                a0 =  (A+1.0) + (A-1.0)*c + 2.0*sqrtf(A)*alpha;
                a1 =  -2.0*( (A-1.0) + (A+1.0)*c);
                a2 =  (A+1.0) + (A-1.0)*c - 2.0*sqrtf(A)*alpha;
                break;

            case HIGH_SHELF:
                b0 = A*( (A+1.0) + (A-1.0)*c + 2.0*sqrtf(A)*alpha );
                b1 = -2.0*A*( (A-1.0) + (A+1.0)*c);
                b2 = A*( (A+1.0) + (A-1.0)*c - 2.0*sqrtf(A)*alpha );
                a0 = (A+1.0) - (A-1.0)*c + 2.0*sqrtf(A)*alpha;
                a1 = 2.0*( (A-1.0) - (A+1.0)*c);
                a2 = (A+1.0) - (A-1.0)*c - 2.0*sqrtf(A)*alpha;
                break;

            default:
                break;
        }

        b0 /=  a0;
        b1 /=  a0;
        b2 /=  a0;
        a1 /=  a0;
        a2 /=  a0;

        cf->b0 = b0;
        cf->b1 = b1;
        cf->b2 = b2;
        //negate 'a' coefficients so addition can be used in filter
        cf->a1 = -a1;
        cf->a2 = -a2;

}

eq_coeffs*
make_eq_band(int type, eq_coeffs* cf, float fs, float f0, float Q, float G)
{

    cf = (eq_coeffs*) malloc(sizeof(eq_coeffs));
    eq_compute_coeffs(cf, type, fs, f0, Q, G);
    cf->y1 = 0.0;
    cf->y2 = 0.0;
    cf->x1 = 0.0;
    cf->x2 = 0.0;
    return cf;

}

void
eq_update_gain(eq_coeffs* cf, float G)
{
    //EFX_PRINT(Serial.printf("band gain: %f\n", G));
    float A = powf(10.0, G/40.0);

    float iA = 1.0/A;
    float sqrtA2 = 2.0*sqrtf(A);
    float a0, a1, a2;
    float b0, b1, b2;
    a0 = a1 = a2 = 0.0;
    b0 = b1 = b2 = 0.0;



    float c = cf->c;
    float alpha = cf->alpha;

        switch(cf->type){

            case PK_EQ:
                b0 =   1.0 + alpha*A;
                b1 =  -2.0*c;
                b2 =   1.0 - alpha*A;
                a0 =   1.0 + alpha*iA;
                a1 =  -2.0*c;
                a2 =   1.0 - alpha*iA;
                break;

            case LOW_SHELF:
                b0 =  A*( (A+1.0) - (A-1.0)*c + sqrtA2*alpha);
                b1 =  2.0*A*( (A-1.0) - (A+1.0)*c);
                b2 =  A*( (A+1.0) - (A-1.0)*c - sqrtA2*alpha);
                a0 =  (A+1.0) + (A-1.0)*c + sqrtA2*alpha;
                a1 =  -2.0*( (A-1.0) + (A+1.0)*c);
                a2 =  (A+1.0) + (A-1.0)*c - sqrtA2*alpha;
                break;

            case HIGH_SHELF:
                b0 = A*( (A+1.0) + (A-1.0)*c + sqrtA2*alpha );
                b1 = -2.0*A*( (A-1.0) + (A+1.0)*c);
                b2 = A*( (A+1.0) + (A-1.0)*c - sqrtA2*alpha );
                a0 = (A+1.0) - (A-1.0)*c + sqrtA2*alpha;
                a1 = 2.0*( (A-1.0) - (A+1.0)*c);
                a2 = (A+1.0) - (A-1.0)*c - sqrtA2*alpha;
                break;

            default:
                break;
        }

		float ia0 = 1.0/a0;
        b0 *=  ia0;
        b1 *=  ia0;
        b2 *=  ia0;
        a1 *=  ia0;
        a2 *=  ia0;

        cf->b0 = b0;
        cf->b1 = b1;
        cf->b2 = b2;
        //negate 'a' coefficients so addition can be used in filter
        cf->a1 = -a1;
        cf->a2 = -a2;

}

}
