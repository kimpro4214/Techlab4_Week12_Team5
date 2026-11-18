#pragma pack_matrix(row_major)

// ParticleSystemComponent Beam/Ribbon 띠 셰이더
// 정점은 CPU에서 이미 월드 좌표로 펼쳐져 온다 (점마다 좌우 2개)

cbuffer constants : register(b0) { matrix VP; };

// ParticleDynamicData.h의 FParticleStripConstants와 바이트 단위로 같아야 한다 (16바이트)
cbuffer ParticleStrip : register(b1)
{
    float HasTexture;         // 1: t0 텍스처를 곱한다
    float SoftEdge;           // 1: 폭 방향 가장자리를 부드럽게 깎는다
    float PremultiplyAlpha;   // 1: Additive(One/One)라 rgb에 알파를 직접 곱한다
    float Padding;
};

Texture2D    StripTexture : register(t0);
SamplerState StripSampler : register(s0);

// ParticleDynamicData.h의 FParticleStripVertex와 순서가 같아야 한다 (36바이트)
struct VS_INPUT
{
    float3 Position : POSITION;   // 월드
    float2 UV       : TEXCOORD0;  // U: 길이 방향, V: 폭 방향 (0 ~ 1)
    float4 Color    : COLOR0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV       : TEXCOORD0;
    float4 Color    : COLOR0;
};

PS_INPUT mainVS(VS_INPUT In)
{
    PS_INPUT Out;
    Out.Position = mul(float4(In.Position, 1.0f), VP);
    Out.UV       = In.UV;
    Out.Color    = In.Color;
    return Out;
}

float4 mainPS(PS_INPUT In) : SV_TARGET
{
    float4 Color = In.Color;
    if (HasTexture > 0.5f)
        Color *= StripTexture.Sample(StripSampler, In.UV);

    // 띠 가운데(V = 0.5)가 가장 진하고 양쪽 끝(V = 0, 1)으로 갈수록 사라진다
    float Edge = saturate(1.0f - abs(In.UV.y * 2.0f - 1.0f));
    Color.a *= lerp(1.0f, Edge, SoftEdge);

    if (PremultiplyAlpha > 0.5f)
        Color.rgb *= Color.a;
    return Color;
}
