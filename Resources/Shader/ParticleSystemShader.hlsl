#pragma pack_matrix(row_major)

cbuffer constants : register(b0)
{
    matrix VP;    
};

cbuffer Worldconstants : register(b2)
{
    matrix World;  
};

Texture2D ParticleTexture : register(t0);
SamplerState ParticleSampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    output.position = mul(mul(float4(input.position, 1.0f), World), VP);
    output.uv = input.uv;
    output.color = input.color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    return ParticleTexture.Sample(ParticleSampler, input.uv) * input.color;
}
