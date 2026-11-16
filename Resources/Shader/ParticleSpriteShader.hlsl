#pragma pack_matrix(row_major)

// ParticleSystemComponent 스프라이트 이미터용 셰이더
// 파티클 하나당 정점 4개. 정점은 파티클 중심만 갖고, 카메라를 향하도록 VS에서 코너를 펼친다

cbuffer constants      : register(b0) { matrix VP; };
cbuffer Worldconstants : register(b2) { matrix World; };   // = SimulationToWorld

// ParticleDynamicData.h의 FParticleSpriteConstants와 바이트 단위로 같아야 한다 (32바이트)
cbuffer ParticleSprite : register(b1)
{
    float3 CameraRight;
    float  SoftCircle;        // 0: 텍스처 그대로, 1: 원형으로 가장자리를 부드럽게 깎는다
    float3 CameraUp;
    float  PremultiplyAlpha;  // 1: Additive(One/One)라 rgb에 알파를 직접 곱한다
};

Texture2D    SpriteTexture : register(t0);
SamplerState SpriteSampler : register(s0);

// ParticleDynamicData.h의 FParticleSpriteVertex와 순서가 같아야 한다 (52바이트)
struct VS_INPUT
{
    float3 Position     : POSITION;    // 시뮬레이션 공간 중심
    float  RelativeTime : TEXCOORD0;
    float2 Size         : TEXCOORD1;
    float2 Corner       : TEXCOORD2;   // -0.5 ~ 0.5
    float  Rotation     : TEXCOORD3;
    float4 Color        : COLOR0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV       : TEXCOORD0;
    float2 Corner   : TEXCOORD1;
    float4 Color    : COLOR0;
};

PS_INPUT mainVS(VS_INPUT In)
{
    // 코너를 회전시킨 뒤 크기만큼 늘린다
    float s, c;
    sincos(In.Rotation, s, c);
    float2 Offset = float2(c * In.Corner.x - s * In.Corner.y,
                           s * In.Corner.x + c * In.Corner.y) * In.Size;

    // 중심은 월드로 옮기고, 코너는 카메라 축으로 펼친다 → 항상 카메라를 향한다
    float3 Center   = mul(float4(In.Position, 1.0f), World).xyz;
    float3 WorldPos = Center + CameraRight * Offset.x + CameraUp * Offset.y;

    PS_INPUT Out;
    Out.Position = mul(float4(WorldPos, 1.0f), VP);
    Out.UV       = float2(In.Corner.x + 0.5f, 0.5f - In.Corner.y);
    Out.Corner   = In.Corner;
    Out.Color    = In.Color;
    return Out;
}

float4 mainPS(PS_INPUT In) : SV_TARGET
{
    float4 Color = SpriteTexture.Sample(SpriteSampler, In.UV) * In.Color;

    float Circle = saturate(1.0f - length(In.Corner) * 2.0f);
    Color.a *= lerp(1.0f, Circle, SoftCircle);

    if (PremultiplyAlpha > 0.5f)
        Color.rgb *= Color.a;
    return Color;
}
