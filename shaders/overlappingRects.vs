struct PushConstants
{
    float2 rectMin;
    float2 rectMax;
    float4 color;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    float x = (vertexID & 1) ? g_push.rectMax.x : g_push.rectMin.x;
    float y = (vertexID >> 1) ? g_push.rectMin.y : g_push.rectMax.y;

    VSOutput output;
    output.position = float4(x, y, 0.0f, 1.0f);
    output.color = g_push.color;
    return output;
}
