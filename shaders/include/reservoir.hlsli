#ifndef RESERVOIR_HLSLI
#define RESERVOIR_HLSLI

#include "../../include/h2/shaders/shaderInterop.h"
#include "rng.hlsli"

void InitializeReservoir(out Reservoir reservoir)
{
    reservoir.lightIndex = 0xffffffff;
    reservoir.contributionWeight = 0.0f;
    reservoir.sampleCount = 0;
    reservoir.weightSum = 0.0f;
}

bool UpdateReservoir(inout Reservoir reservoir, uint lightIndex, float weight, inout RngState rngState)
{
    bool candidateAccepted = false;
    
    reservoir.weightSum += weight;
    reservoir.sampleCount++;

    if (weight > 0.0f && NextRandomFloat(rngState) < (weight / reservoir.weightSum))
    {
        reservoir.lightIndex = lightIndex;
        candidateAccepted = true;
    }

    return candidateAccepted;
}

#endif // RESERVOIR_HLSLI
