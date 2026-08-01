#ifndef OCTAHEDRAL_HLSLI
#define OCTAHEDRAL_HLSLI

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

#endif // OCTAHEDRAL_HLSLI
