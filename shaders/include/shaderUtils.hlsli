#ifndef SHADERUTILS_HLSLI
#define SHADERUTILS_HLSLI

float3 LinearToSrgb(float3 linearColor)
{
    linearColor = max(linearColor, 0.0f);

    float3 low = linearColor * 12.92f;
    float3 high = 1.055f * pow(linearColor, 1.0f / 2.4f) - 0.055f;

    return lerp(low, high, step(0.0031308f, linearColor));
}

float2 EncodeOctahedral(float3 normal)
{
    normal /= (abs(normal.x) + abs(normal.y) + abs(normal.z));

    if (normal.z >= 0.0f)
    {
        return normal.xy;
    }

    float2 wrapped;
    wrapped.x = (1.0f - abs(normal.y)) * (normal.x >= 0.0f ? 1.0f : -1.0f);
    wrapped.y = (1.0f - abs(normal.x)) * (normal.y >= 0.0f ? 1.0f : -1.0f);

    return wrapped;
}

float3 DecodeOctahedral(float2 encoded)
{
    float3 normal = float3(encoded.x, encoded.y, 1.0f - abs(encoded.x) - abs(encoded.y));

    float fold = saturate(-normal.z);
    normal.x += normal.x >= 0.0f ? -fold : fold;
    normal.y += normal.y >= 0.0f ? -fold : fold;

    return normalize(normal);
}

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

float3 DebugColorFromId(uint id)
{
    uint hash = id * 2654435761u;
    return float3(
        float((hash >> 0) & 0xFF) / 255.0f,
        float((hash >> 8) & 0xFF) / 255.0f,
        float((hash >> 16) & 0xFF) / 255.0f);
}

#endif // SHADERUTILS_HLSLI
