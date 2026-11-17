#pragma pack_matrix(row_major)

// ParticleSystemComponent 메시 이미터용 셰이더 (인스턴싱)
// 메시 하나를 파티클 수만큼 한 번에 그린다. 파티클별 행렬과 색은 StructuredBuffer에서 SV_InstanceID로 읽는다
// 씬 조명 대신 고정 방향광 하나로 간단히 음영을 넣는다

cbuffer constants : register(b0) { matrix VP; };

// ParticleDynamicData.h의 FParticleMeshInstance와 바이트 단위로 같아야 한다 (80바이트)
struct FParticleMeshInstance
{
    float4x4 World;   // 메시 로컬 → 월드
    float4   Color;
};
StructuredBuffer<FParticleMeshInstance> Instances : register(t16);

// 스태틱 메시 정점(FVertex)과 순서가 같아야 한다. 입력 레이아웃이 이 구조체로 만들어진다
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float4 Tangent  : TANGENT;
    float2 UV       : TEXCOORD;
    float4 Color    : COLOR;
};

struct PS_INPUT
{
    float4 Position    : SV_POSITION;
    float3 WorldNormal : NORMAL;
    float4 Color       : COLOR0;
};

PS_INPUT mainVS(VS_INPUT In, uint InstanceID : SV_InstanceID)
{
    const FParticleMeshInstance Instance = Instances[InstanceID];

    PS_INPUT Out;
    const float4 WorldPos = mul(float4(In.Position, 1.0f), Instance.World);
    Out.Position    = mul(WorldPos, VP);
    // 균일하지 않은 크기에서는 정확하지 않지만, 정규화만 해도 파티클 음영에는 충분하다
    Out.WorldNormal = normalize(mul(In.Normal, (float3x3)Instance.World));
    Out.Color       = Instance.Color * In.Color;
    return Out;
}

float4 mainPS(PS_INPUT In) : SV_TARGET
{
    const float3 LightDir = normalize(float3(0.4f, 0.3f, 0.85f));   // 위쪽에서 비추는 고정 방향광
    const float  Ambient  = 0.35f;
    const float  Diffuse  = saturate(dot(normalize(In.WorldNormal), LightDir));

    return float4(In.Color.rgb * (Ambient + (1.0f - Ambient) * Diffuse), In.Color.a);
}
