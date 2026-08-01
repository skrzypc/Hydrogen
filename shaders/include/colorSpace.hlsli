#ifndef COLORSPACE_HLSLI
#define COLORSPACE_HLSLI

float3 LinearToSrgb(float3 linearColor)
{
    linearColor = max(linearColor, 0.0f);

    float3 low = linearColor * 12.92f;
    float3 high = 1.055f * pow(linearColor, 1.0f / 2.4f) - 0.055f;

    return lerp(low, high, step(0.0031308f, linearColor));
}

#endif // COLORSPACE_HLSLI
