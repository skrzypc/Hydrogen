#ifndef COMMON_HLSLI
#define COMMON_HLSLI

#include "../../include/shaderInterop.h"

static const float INFINITY = asfloat(0x7F800000);
static const float kPi = 3.14159265359f;

ConstantBuffer<FrameData> g_frame : register(b2, space0);

#endif // COMMON_HLSLI
