#ifndef RNG_HLSLI
#define RNG_HLSLI

typedef uint RngState;

// Based on Ray Tracing Gems II, Chapter 14.

uint JenkinsHash(uint x)
{
    x += x << 10;
    x ^= x >> 6;
    x += x << 3;
    x ^= x >> 11;
    x += x << 15;
    
    return x;
}

uint Xorshift(inout uint x)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    
    return x;
}

float uintToFloat(uint x)
{
    return asfloat(0x3f800000 | (x >> 9)) - 1.f;
}

RngState InitRng(uint2 pixelCoord, uint2 resolution, uint frameNumber)
{
    uint rngState = dot(pixelCoord, uint2(1, resolution.x)) ^ JenkinsHash(frameNumber);
    
    return JenkinsHash(rngState);
}

uint NextRandomUint(inout RngState state)
{
    return Xorshift(state);
}

float NextRandomFloat(inout RngState state)
{
    return uintToFloat(Xorshift(state));
}

#endif // RNG_HLSLI
