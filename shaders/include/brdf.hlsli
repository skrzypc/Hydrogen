#ifndef BRDF_HLSLI
#define BRDF_HLSLI

#include "common.hlsli"
#include "sampling.hlsli"
#include "shaderUtils.hlsli"

struct Surface
{
    float3 albedo;
    float roughness;
    float metallic;
};

struct BrdfSample
{
    float3 direction;
    float3 weight;
    bool isValid;
};

// LAMBERT MATERIAL

float3 EvaluateBrdfLambert(Surface surface)
{
    return surface.albedo / kPi;
}

BrdfSample SampleBrdfLambert(Surface surface, float3 N, float2 u)
{
    BrdfSample brdfSample = (BrdfSample) 0;

    float3 randomSampleDirection = SampleCosineHemisphere(u);

    float3 T, B;
    OrthonormalBasis(T, N, B);

    brdfSample.direction = normalize(randomSampleDirection.x * T + randomSampleDirection.y * N + randomSampleDirection.z * B);

    // Math
    //      cosTheta = dot(brdfSample.direction, N)
    //      brdf = albedo / kPi [Lambert]
    //      brdfPdf = cosTheta / kPi [Cosine Weighted Hemisphere]
    //      
    //      weight = brdf * cosTheta / brdfPdf <==>
    //      weight = (albedo / kPi) * cosTheta / (cosTheta / kPi) <==>
    //      weight = albedo

    brdfSample.weight = surface.albedo;
    brdfSample.isValid = true;

    return brdfSample;
}

// GGX MATERIAL

// Trowbridge-Reitz 1975, "Average Irregularity Representation of a Rough Surface for Ray Reflection".
// Brought to graphics as GGX by Walter et al 2007, "Microfacet Models for Refraction through Rough Surfaces".
float DistributionGgx(float NoH, float alpha)
{
    float alphaSquared = alpha * alpha;
    float denominator = NoH * NoH * (alphaSquared - 1.0f) + 1.0f;

    return alphaSquared / (kPi * denominator * denominator);
}

// Smith 1967, "Geometrical Shadowing of a Random Rough Surface".
// Height correlated form from Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs",
// which shows the separable form underestimates G because masking and shadowing both correlate with height.
// Already divided by 4 * NoV * NoL.
float VisibilitySmithGgx(float NoV, float NoL, float alpha)
{
    float alphaSquared = alpha * alpha;

    float lambdaV = NoL * sqrt(NoV * NoV * (1.0f - alphaSquared) + alphaSquared);
    float lambdaL = NoV * sqrt(NoL * NoL * (1.0f - alphaSquared) + alphaSquared);

    return 0.5f / max(lambdaV + lambdaL, 1e-5f);
}

// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering".
float3 FresnelSchlick(float VoH, float3 f0)
{
    return f0 + (1.0f - f0) * pow(saturate(1.0f - VoH), 5.0f);
}

float3 EvaluateBrdfGgx(Surface surface, float3 N, float3 V, float3 L)
{
    float3 H = normalize(V + L);

    float NoL = saturate(dot(N, L));
    float NoV = max(abs(dot(N, V)), 1e-4f);
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));

    float alpha = surface.roughness * surface.roughness;

    alpha = max(alpha, 0.001f);
    
    // Dielectrics reflect 4% head on, metals use their albedo and have no diffuse.
    float3 f0 = lerp(0.04f, surface.albedo, surface.metallic);
    float3 F = FresnelSchlick(VoH, f0);

    float3 diffuseTerm = (1.0f - F) * (1.0f - surface.metallic) * surface.albedo / kPi;
    // D(H) * G2(V, L) * F / (4.0f * NoV * NoL)
    // Division is included in the geometry term (visibility).
    float3 specularTerm = DistributionGgx(NoH, alpha) * VisibilitySmithGgx(NoV, NoL, alpha) * F;

    return diffuseTerm + specularTerm;
}

BrdfSample SampleBrdfGgx(Surface surface, float3 N, float3 V, float2 u, bool isSpecular)
{
    BrdfSample brdfSample = (BrdfSample) 0;

    if (dot(N, V) <= 0.0f)
    {
        brdfSample.isValid = false;

        return brdfSample;
    }
    
    float3 T, B;
    OrthonormalBasis(T, N, B);
    
    if (!isSpecular)
    {
        float3 randomSampleDirection = SampleCosineHemisphere(u);

        brdfSample.direction = normalize(randomSampleDirection.x * T + randomSampleDirection.y * N + randomSampleDirection.z * B);

        // Math
        //      General equation:
        //          weight = f * cosTheta / pdf
        //
        //      f is the diffuse term of EvaluateBrdfGgx:
        //          f = (1 - F) * (1 - metallic) * albedo / kPi
        //      cosTheta uses the shading normal:
        //          cosTheta = NoL = dot(N, L)
        //      pdf of the cosine weighted hemisphere:
        //          pdf = NoL / kPi
        //
        //      Substituting:
        //          weight = ((1 - F) * (1 - metallic) * albedo / kPi) * NoL / (NoL / kPi)
        //          weight = (1 - F) * (1 - metallic) * albedo
        //      kPi and NoL cancel.
        //
        //      The caller divides the weight by the diffuse selection probability.

        float3 H = normalize(V + brdfSample.direction);
        float VoH = saturate(dot(V, H));
        float3 f0 = lerp(0.04f, surface.albedo, surface.metallic);
        float3 F = FresnelSchlick(VoH, f0);
        
        brdfSample.weight = (1.0f - F) * (1.0f - surface.metallic) * surface.albedo;
    }
    else
    {
        float alpha = surface.roughness * surface.roughness;
        alpha = max(alpha, 0.001f);

        // Transform V from world to local space!
        // V transformed from world to local space with inverse TNB (inverse TNB == transposed TNB)
        float3 Vl = float3(dot(T, V), dot(N, V), dot(B, V));
        float3 randomMicrofacetNormal = SampleGGXVNDF(Vl, alpha, u);

        // L = reflect(-V, H)
        // H is the sampled microfacet normal, which equals the half vector normalize(V + L).
        // VNDF sampling returns it in local space, so it is transformed to world space with TNB.
        brdfSample.direction = reflect(-V, normalize(randomMicrofacetNormal.x * T + randomMicrofacetNormal.y * N + randomMicrofacetNormal.z * B));

        if (dot(N, brdfSample.direction) <= 0.0f)
        {
            brdfSample.isValid = false;

            return brdfSample;
        }

        // Math
        //      General equation:
        //          weight = f * cosTheta / pdf
        //
        //      f is the specular GGX term of EvaluateBrdfGgx:
        //          f = D(H) * G2(V, L) * F / (4 * NoV * NoL)
        //      cosTheta uses the shading normal, not the sampled microfacet normal H:
        //          cosTheta = NoL = dot(N, L)
        //
        //      pdf is the density of L, but VNDF sampling gives the density of H:
        //          pdfH = Dv(H) = G1(V) * max(0, VoH) * D(H) / NoV
        //      Both describe the same probability mass:
        //          pdfL * dwL = pdfH * dwH
        //      L = reflect(-V, H) sweeps twice the angle that H does, so:
        //          dwL = 4 * VoH * dwH
        //      Therefore:
        //          pdfL = pdfH / (4 * VoH)
        //          pdfL = G1(V) * VoH * D(H) / (NoV * 4 * VoH)
        //          pdfL = G1(V) * D(H) / (4 * NoV)
        //
        //      Substituting:
        //          weight = D(H) * G2(V, L) * F / (4 * NoV * NoL) * NoL / (G1(V) * D(H) / (4 * NoV))
        //          weight = F * G2(V, L) / G1(V)
        //      D(H), NoL and 4 * NoV cancel. F uses VoH from the sampled microfacet normal.
        //
        //      Height correlated Smith, with SV and SL the square roots from VisibilitySmithGgx:
        //          G1(V) = 2 * NoV / (NoV + SV)
        //          G2(V, L) = 2 * NoV * NoL / (SV * NoL + SL * NoV)
        //          G2(V, L) / G1(V) = NoL * (SV + NoV) / (SV * NoL + SL * NoV)
        //
        //      The caller divides the weight by the specular selection probability.

        float3 H = normalize(V + brdfSample.direction);
        float VoH = saturate(dot(V, H));
        float3 f0 = lerp(0.04f, surface.albedo, surface.metallic);
        float3 F = FresnelSchlick(VoH, f0);

        float alphaSquared = alpha * alpha;

        float NoL = saturate(dot(N, brdfSample.direction));
        float NoV = max(abs(dot(N, V)), 1e-4f);
        
        float SV = sqrt(NoV * NoV * (1.0f - alphaSquared) + alphaSquared);
        float SL = sqrt(NoL * NoL * (1.0f - alphaSquared) + alphaSquared);

        brdfSample.weight = F * NoL * (SV + NoV) / (SV * NoL + SL * NoV);
    }

    brdfSample.isValid = true;

    // Result have to be divided by probability of selecting diffuse|specular!
    return brdfSample;
}

#endif // BRDF_HLSLI
