#ifndef SAMPLING_HLSLI
#define SAMPLING_HLSLI

#include "common.hlsli"

// Cosine weighted hemisphere sampling.
float3 SampleCosineHemisphere(float2 u)
{
    float r = sqrt(u.x);
    float phi = 2.0f * kPi * u.y;

    float x, y, z;
    sincos(phi, z, x);
    x *= r;
    z *= r;
    y = sqrt(max(0.0f, 1.0f - u.x));

    return float3(x, y, z);
}

#endif // SAMPLING_HLSLI
