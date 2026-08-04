#ifndef BRDF_HLSLI
#define BRDF_HLSLI

static const float kPi = 3.14159265f;

struct Surface
{
    float3 albedo;
    float roughness;
    float metalness;
};

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

// Cook and Torrance 1982, "A Reflectance Model for Computer Graphics".
// Returns the BRDF value. Incident radiance and NoL still needs to be applied.
float3 EvaluateBrdf(Surface surface, float3 N, float3 V, float3 L)
{
    float3 H = normalize(V + L);

    float NoL = saturate(dot(N, L));
    float NoV = max(abs(dot(N, V)), 1e-4f);
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));

    float alpha = surface.roughness * surface.roughness;

    // Dielectrics reflect 4% head on, metals use their albedo and have no diffuse.
    float3 f0 = lerp(0.04f, surface.albedo, surface.metalness);
    float3 F = FresnelSchlick(VoH, f0);

    float3 specular = DistributionGgx(NoH, alpha) * VisibilitySmithGgx(NoV, NoL, alpha) * F;
    float3 diffuse = (1.0f - F) * (1.0f - surface.metalness) * surface.albedo / kPi;

    return diffuse + specular;
}

#endif // BRDF_HLSLI
