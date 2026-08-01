#ifndef TONEMAP_HLSLI
#define TONEMAP_HLSLI

// Narkowicz fit of the ACES filmic curve.
float3 TonemapAces(float3 color)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

#endif // TONEMAP_HLSLI
