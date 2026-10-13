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

// "Sampling the GGX Distribution of Visible Normals", Eric Heitz
// https://jcgt.org/published/0007/04/01/
// V: view direction.
// alphaX, alphaZ: roughness parameters.
// u: random values.
// Output: N, normal sampled with PDF D_V(N) = G1(V) * max(0, dot(V, N)) * D(N) / V.y
//
// Warning: Heitz's reference assumes Z up; this variant is adapted to Y up (normal on +Y, tangent plane XZ).
float3 SampleGGXVNDFAnisotropic(float3 V, float alphaX, float alphaZ, float2 u)
{
    // Section 3.2: transforming the view direction to the hemisphere configuration.
    float3 Vh = normalize(float3(alphaX * V.x, V.y, alphaZ * V.z));

    // Section 4.1: orthonormal basis (with specialcase if cross product is zero).
    float lengthSquared = Vh.x * Vh.x + Vh.z * Vh.z;
    float3 T1 = lengthSquared > 0.0f ? float3(-Vh.z, 0.0f, Vh.x) * rsqrt(lengthSquared) : float3(1.0f, 0.0f, 0.0f);
    float3 T2 = cross(T1, Vh);

    // Section 4.2: parameterization of the projected area.
    float r = sqrt(u.x);
    float phi = 2.0 * kPi * u.y;
    float t1, t2;
    sincos(phi, t2, t1);
    t1 *= r;
    t2 *= r;
    float s = 0.5f * (1.0f + Vh.y);
    t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;

    // Section 4.3: reprojection onto hemisphere.
    float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;
    
    // Section 3.4: transforming the normal back to the ellipsoid configuration.
    float3 N = normalize(float3(alphaX * Nh.x, max(0.0, Nh.y), alphaZ * Nh.z));
    
    return N;
}

float3 SampleGGXVNDF(float3 V, float alpha, float2 u)
{
    return SampleGGXVNDFAnisotropic(V, alpha, alpha, u);
}

#endif // SAMPLING_HLSLI
