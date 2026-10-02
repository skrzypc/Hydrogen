#ifndef COMMON_HLSLI
#define COMMON_HLSLI

#include "../../include/shaderInterop.h"

static const float INFINITY = asfloat(0x7F800000);
static const float kPi = 3.14159265359f;

ConstantBuffer<FrameData> g_frame : register(b2, space0);

SamplerState LinearWrapSampler : register(s0);
SamplerState LinearClampSampler : register(s1);
SamplerState PointClampSampler : register(s2);
SamplerState AnisoWrapSampler : register(s3);
SamplerState LinearBorderSampler : register(s4);

#endif // COMMON_HLSLI
