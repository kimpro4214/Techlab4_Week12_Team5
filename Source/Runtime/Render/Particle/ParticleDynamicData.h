#pragma once

#include "Render/Particle/ParticleHelper.h"
#include "Render/RenderPacket.h"
#include "Render/Buffer.h"


class UMaterial;

// 코너마다 1개씩(파티클당 4개) 올린다. 
// ParticleSpriteShader.hlsl의 VS_INPUT과 필드 순서, 크기가 정확히 같아야 한다.
struct FParticleSpriteVertex
{
	FVector  Position;      // 0   파티클 중심 (시뮬레이션 공간. VS가 World(=SimulationToWorld)를 곱한다)
	float    RelativeTime;  // 12  수명 진행도
	FVector2 Size;          // 16  가로·세로 크기
	FVector2 Corner;        // 24  쿼드 코너 (-0.5 ~ 0.5). UV로도 쓴다
	float    Rotation;      // 32  화면 기준 회전(라디안)
	FVector4 Color;         // 36
};
static_assert(sizeof(FParticleSpriteVertex) == 52, "ParticleSpriteShader.hlsl VS_INPUT과 맞춰야 한다");


// ParticleSpriteShader.hlsl의 cbuffer ParticleSprite(b1)과 바이트 단위로 같아야 한다.
struct FParticleSpriteConstants
{
	FVector CameraRight = FVector(0.0f, 1.0f, 0.0f);   // 0   카메라 오른쪽 벡터
	float SoftCircle = 1.0f;      // 12  소프트 서클(0~1)
	FVector CameraUp = FVector(0.0f, 0.0f, 1.0f);      // 16  카메라 위쪽 벡터
	float PremultiplyAlpha = 0.0f;   // 28  1이면 rgb에 알파를 곱한다 (Additive가 One/One이라 필요)
};
static_assert(sizeof(FParticleSpriteConstants) == 32, "ParticleSpriteShader.hlsl cbuffer ParticleSprite과 맞춰야 한다");


// ReplayData(Source)를 받아 렌더 패킷을 만들어서 렌더 큐로 전달
struct FDynamicEmitterDataBase
{
	virtual ~FDynamicEmitterDataBase() = default;

	int32 EmitterIndex = 0; // 몇 번째 Emiiter 인지.

	virtual const FDynamicEmitterReplayDataBase& GetSource() const = 0;

	// 뷰 하나에 대해 렌더 패킷을 만든다 (UE: GetDynamicMeshElementsEmitter).
	// 뷰포트(4분할 등)마다 한 번씩 불린다.
	virtual void Submit(FRenderQueue& RenderQueue, const FViewContext& View) = 0;
};

// 2D 스프라이트 Emitter 용
struct FDynamicSpriteEmitterData : public FDynamicEmitterDataBase
{
	FDynamicSpriteEmitterReplayDataBase Source;

	const FDynamicEmitterReplayDataBase& GetSource() const override { return Source; }
	virtual int32 GetDynamicVertexStride() const { return sizeof(FParticleSpriteVertex); }

	// 매 프레임 Source를 갱신한 직후 호출한다. 버퍼 용량을 맞추고, 뷰 버퍼 사용 위치를 처음으로 되돌린다.
	void Init(int32 MaxParticles);

	void Submit(FRenderQueue& RenderQueue, const FViewContext& View) override;
private:

	// 뷰 하나가 쓰는 GPU 자원.
	// 한 프레임에 여러 뷰가 제출된 뒤 한꺼번에 그려질 수 있으므로, 버퍼 하나를 뷰마다 덮어쓰면 안 된다.
	struct FViewResources
	{
		TUniquePtr<FVertexBuffer> VertexBuffer;   // D3D11_USAGE_DYNAMIC + CPU_ACCESS_WRITE
		FParticleSpriteConstants  Constants;      // 렌더 패킷이 포인터로 참조하므로 프레임 동안 살아있어야 한다
	};

	void BuildIndexBuffer(int32 MaxParticles);
	FViewResources* AcquireViewResources();

	static constexpr int32 MaxViewsPerFrame = 8;

	TArray<TUniquePtr<FViewResources>> ViewResources;  // 배열이 커져도 주소가 안 바뀌도록 포인터로 보관
	int32 NextViewIndex = 0;
	int32 Capacity = 0;                                 // 버퍼가 담을 수 있는 최대 파티클 수
	TUniquePtr<FIndexBuffer> IndexBuffer;               // 쿼드 인덱스는 변하지 않으니 Static, 모든 뷰가 공유
	TArray<FParticleSpriteVertex> ScratchVertices;      // 정점 임시 배열. 매 프레임 할당하지 않도록 재사용

	// 정렬은 시뮬레이션 배열을 건드리지 않고 (깊이, 활성 슬롯 번호)만 따로 정렬한다
	struct FParticleOrder
	{
		float Depth;
		int32 Slot;   // ParticleIndices의 몇 번째인지
	};
	void BuildDrawOrder(int32 Count, const FViewContext& View);
	TArray<FParticleOrder> DrawOrder;                    // 뷰마다 다시 채운다. 재사용
};

// 인스턴스 하나(파티클 하나)의 데이터. ParticleMeshShader.hlsl의 FParticleMeshInstance와 같아야 한다 (80바이트)
struct FParticleMeshInstance
{
	FMatrix  World;   // 메시 로컬 → 월드
	FVector4 Color;
};
static_assert(sizeof(FParticleMeshInstance) == 80, "ParticleMeshShader.hlsl과 크기가 달라지면 안 된다");

// 스태틱 메시 Emitter 용. 메시 자체 VB/IB를 쓴다
// - Required 재질이 ParticleMesh이면: 인스턴스 버퍼에 파티클 행렬·색을 올리고 섹션마다 DrawIndexedInstanced 한 번
// - 아니면: 메시 자체 재질(씬 조명)로 파티클 하나 × 섹션 하나 = 패킷 하나
struct FDynamicMeshEmitterData : public FDynamicEmitterDataBase
{
	FDynamicMeshEmitterReplayData Source;

	const FDynamicEmitterReplayDataBase& GetSource() const override { return Source; }

	// 매 프레임 Source를 갱신한 직후 호출한다. 인스턴싱이면 인스턴스 버퍼를 채워 올린다
	void Init();

	void Submit(FRenderQueue& RenderQueue, const FViewContext& View) override;

private:
	bool UsesInstancing() const;

	// 파티클 하나의 메시 로컬 → 시뮬레이션 공간 행렬 (크기, 방향, 위치)
	FMatrix BuildParticleMatrix(const FBaseParticle& Particle) const;

	void SubmitInstanced(FRenderQueue& RenderQueue, const FViewContext& View);
	void SubmitPerParticle(FRenderQueue& RenderQueue, const FViewContext& View);

	TUniquePtr<FStructuredBuffer> InstanceBuffer;   // 모든 뷰가 같이 쓴다 (행렬이 뷰와 무관)
	int32 InstanceCapacity = 0;
	int32 UploadedCount = 0;                        // 이번 프레임에 올린 인스턴스 수
	TArray<FParticleMeshInstance> ScratchInstances; // 매 프레임 할당하지 않도록 재사용
};

// 어떤 DynamicData를 만들지는 EmitterInstance 서브클래스의 GetDynamicData()가 정한다 (팩토리 함수 없음)
