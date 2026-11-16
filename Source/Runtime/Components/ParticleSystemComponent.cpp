#include "EnginePCH.h"
#include "Components/ParticleSystemComponent.h"

#include "Asset/AssetManager.h"					
#include "Render/Particle/ParticleHelper.h"			
#include "Render/Particle/ParticleModuleRequired.h"

void UParticleSystemComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext)
{
	if (!SpriteBatcher || EmitterInstances.IsEmpty()) return;	// Template/유효 Emitter가 없으면 Batcher가 null

	// 기존 빌보드(FViewContext::BuildBillboardMatrix)와 같은 축: Right = Up(월드) x Forward, Up = Forward x Right
	FVector CamForward = ViewContext.CameraForward.Normalized();
	FVector CamRight = FVector::Cross(FVector(0, 0, 1), CamForward).Normalized();
	if (CamRight.Length() <= 1e-6f) CamRight = FVector(0, 1, 0);	// 수직으로 내려다보거나 올려다볼 때
	FVector CamUp = FVector::Cross(CamForward, CamRight).Normalized();

	SpriteBatcher->Begin();

	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		int32 Start = SpriteBatcher->GetSpriteCount();
		FParticleEmitterInstance* Owner = Instance;
		BEGIN_UPDATE_LOOP
			SpriteBatcher->AddSprite(Particle.Location,
				FVector2(Particle.Size.X, Particle.Size.Y),
				FVector4(Particle.Color),
				CamRight,
				CamUp);
		END_UPDATE_LOOP

		int32 Count = SpriteBatcher->GetSpriteCount() - Start;
		if (Count == 0) continue;

		UParticleModuleRequired* Required = Instance->CurrentLODLevel->GetRequiredModule();

		// Required에 재질이 없으면 AssetManager에 등록된 기본 파티클 재질 사용
		UMaterial* Material = Required->GetMaterial();
		if (Material == nullptr) Material = UAssetManager::GetAssetByPath<UMaterial>("ParticleMaterial");
		if (Material == nullptr) continue;

		// LocalSpace: 파티클이 Component 로컬 좌표 → Component 행렬로 월드 변환
		// WorldSpace: Spawn 때 이미 월드 좌표로 저장됨 → 단위 행렬 (Component 행렬을 곱하면 이중 변환)
		const FMatrix Model = Required->IsUseLocalSpace() ? GetWorldMatrix() : FMatrix::Identity;

		FRenderPacket RenderPac;	// Mesh / Proxy는 null 유지: Proxy가 있으면 Model 대신 Proxy 행렬이 쓰임 (PacketWorld)

		RenderPac.Geometry = SpriteBatcher->GetGeometry();
		RenderPac.StartIndex = Start * 6;		// 이 Emitter 구간의 시작 인덱스 (Sprite당 인덱스 6개)
		RenderPac.IndexCount = Count * 6;		// 0이면 IB 전체를 그리므로 반드시 채움
		RenderPac.Material = Material;
		RenderPac.Model = RenderQueue.StoreWorldMatrix(Model);	// 패킷은 포인터만 보관 → 큐가 소유한 저장소에 복사
		const FVector Delta = GetWorldLocation() - ViewContext.CameraPosition;
		RenderPac.CameraToParticleDistance = Delta.Dot(Delta);	// 반투명 패킷 거리 정렬용 (거리 제곱)

		RenderQueue.Add(RenderPac);
	}

	SpriteBatcher->End();
}

FBox UParticleSystemComponent::CalcLocalBounds() const
{
	// TODO: 파티클이 퍼지는 범위를 덮는 넉넉한 박스로 교체
	return Super::CalcLocalBounds();
}
