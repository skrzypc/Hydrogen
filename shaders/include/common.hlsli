#ifndef COMMON_HLSLI
#define COMMON_HLSLI

// Matches Hydrogen::ShaderInterop::FrameData in shaderInterop.h
struct FrameData
{
    uint  vertexPositionBufferIndex;
    uint  vertexNormalBufferIndex;
    uint  vertexUvBufferIndex;
    uint  transformBufferIndex;
    uint  viewBufferIndex;
    uint  mainViewIndex;
    float time;
    float deltaTime;
    uint  frameNumber;
    uint  _pad0;
};

// Matches Hydrogen::ShaderInterop::ViewData in shaderInterop.h
struct ViewData
{
    float4x4 viewMx;
    float4x4 projectionMx;
    float4x4 viewProjectionMx;
    float4x4 invViewProjectionMx;

    float3 worldPosition;
    float  nearPlane;

    float3 worldDirection;
    float  farPlane;

    float2 viewportSize;
    uint2  _pad;
};

ConstantBuffer<FrameData> g_frame : register(b2, space0);

#endif // COMMON_HLSLI
