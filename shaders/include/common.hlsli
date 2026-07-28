#ifndef COMMON_HLSLI
#define COMMON_HLSLI

#include "../../include/shaderInterop.h"

const float INFINITY = asfloat(0x7F800000);

ConstantBuffer<FrameData> g_frame : register(b2, space0);

#endif // COMMON_HLSLI
