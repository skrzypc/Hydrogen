#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "rng.hlsli"

static const float kRangeFalloffStartFraction = 0.8f;
static const uint kRisCandidatesLightsCount = 8;

// Distance is -1 for directional lights, matching GetLightContribution/GetLightContributionPT's convention.
void GetLightDirectionAndDistance(GpuLight light, float3 surfaceWorldPosition, out float3 lightDirection, out float lightDistance)
{
    if (light.type == LightTypeDirectional)
    {
        lightDirection = -light.direction;
        lightDistance = -1.0f;
    }
    else
    {
        float3 toLight = light.position - surfaceWorldPosition;
        lightDistance = length(toLight);
        lightDirection = toLight / lightDistance;
    }
}

float CalculateLuminance(const float3 input)
{
    return (0.2126f * input.r + 0.7152f * input.g + 0.0722f * input.b);
}

float3 GetLightContribution(const GpuLight light, const float3 lightDirection, const float lightDistance)
{
    float attenuation = 1.0f;
    float softRangeFalloffFactor = 1.0f;

    if (light.type != LightTypeDirectional)
    {
        if (lightDistance > light.range)
        {
            return float3(0.0f, 0.0f, 0.0f);
        }

        float softRangeFalloffStart = light.range * kRangeFalloffStartFraction;
        float softRangeFalloffEnd = light.range;
        float softRangeFalloffWidth = max(softRangeFalloffEnd - softRangeFalloffStart, 1e-4f);
        float softRangeFalloff = saturate(1.0f - ((lightDistance - softRangeFalloffStart) / softRangeFalloffWidth));

        softRangeFalloffFactor = softRangeFalloff * softRangeFalloff;

        float distanceSquared = max(lightDistance * lightDistance, 1e-6f);
        attenuation = 1.0f / distanceSquared;

        if (light.type == LightTypeSpot)
        {
            float cosAngle = dot(-lightDirection, light.direction);
            float coneFalloff = max(light.cosInnerConeAngle - light.cosOuterConeAngle, 1e-4f);
            attenuation *= saturate((cosAngle - light.cosOuterConeAngle) / coneFalloff);
        }
    }

    return light.color * light.intensity * attenuation * softRangeFalloffFactor;
}

// Don't use range parameter to avoid bias.
float3 GetLightContributionPT(const GpuLight light, const float3 lightDirection, const float lightDistance)
{
    float attenuation = 1.0f;

    if (light.type != LightTypeDirectional)
    {
        float distanceSquared = max(lightDistance * lightDistance, 1e-6f);

        // Cem Yuksel's improved attenuation avoiding singularity at distance=0
        // Source: http://www.cemyuksel.com/research/pointlightattenuation/
        const float radius = 0.05f;
        float rSquared = radius * radius;
        attenuation = 2.0f / (distanceSquared + rSquared + lightDistance * sqrt(distanceSquared + rSquared));

        if (light.type == LightTypeSpot)
        {
            float cosAngle = dot(-lightDirection, light.direction);
            float coneFalloff = max(light.cosInnerConeAngle - light.cosOuterConeAngle, 1e-4f);
            attenuation *= saturate((cosAngle - light.cosOuterConeAngle) / coneFalloff);
        }
    }

    return light.color * light.intensity * attenuation;
}

bool SampleLightUniformly(inout RngState rngState, out GpuLight lightSample, out float lightSampleWeight)
{
    lightSample = (GpuLight) 0;
    lightSampleWeight = 0.0f;

    if (g_frame.lightCount == 0)
    {
        return false;
    }

    StructuredBuffer<GpuLight> lights = ResourceDescriptorHeap[g_frame.lightBufferIndex];
    uint randomLightIndex = min(g_frame.lightCount - 1, uint(NextRandomFloat(rngState) * g_frame.lightCount));
    lightSample = lights[randomLightIndex];
    lightSampleWeight = float(g_frame.lightCount);

    return true;
}

bool SampleLightRIS(inout RngState rngState, const float3 surfaceWorldPosition, const float3 surfaceNormal, out GpuLight lightSample, out float lightSampleWeight)
{
    lightSample = (GpuLight) 0;
    lightSampleWeight = 0.0;
    
    if (g_frame.lightCount == 0)
    {
        return false;
    }
    
    float totalRisWeight = 0.0f;
    float sampleTargetPdf = 0.0f;
    
    for (uint i = 0; i < kRisCandidatesLightsCount; ++i)
    {
        GpuLight candidateLightSample;
        float candidateLightSampleWeight;
        if (SampleLightUniformly(rngState, candidateLightSample, candidateLightSampleWeight))
        {
            float3 lightDirection;
            float lightDistance;
            GetLightDirectionAndDistance(candidateLightSample, surfaceWorldPosition, lightDirection, lightDistance);

            if (dot(surfaceNormal, lightDirection) < 0.00001f) continue;
            
            float candidateTargetPdf = CalculateLuminance(GetLightContributionPT(candidateLightSample, lightDirection, lightDistance));
            const float candidateRisWeight = candidateTargetPdf * candidateLightSampleWeight; // p_hat / pdf
            
            totalRisWeight += candidateRisWeight;
            if (NextRandomFloat(rngState) < (candidateRisWeight / totalRisWeight))
            {
                lightSample = candidateLightSample;
                sampleTargetPdf = candidateTargetPdf;
            }
        }
    }
    
    if (totalRisWeight == 0.0f)
    {
        return false;
    }
    
    lightSampleWeight = (totalRisWeight / float(kRisCandidatesLightsCount)) / sampleTargetPdf;
    
    return true;
}

#endif // COMMON_HLSLI
