#include "EnginePCH.h"
#include "Components/ParticleSystemComponent.h"

#include "Asset/AssetManager.h"					
#include "Render/Particle/ParticleHelper.h"			
#include "Render/Particle/ParticleModules.h"	// Required + Bounds 계산용 Location/Velocity/Lifetime/Size
#include <algorithm>

void UParticleSystemComponent::InitializeSystem()
{
	ResetSystem();
	if (!Template) return;

	int32 MaxParticleCount = 0;
	for (UParticleEmitter* Emitter : Template->GetEmitterArray())
	{
		FParticleEmitterInstance* EmitterInstance = new FParticleEmitterInstance();
		if (EmitterInstance->Init(Emitter, this))
		{
			EmitterInstances.Add(EmitterInstance);
			MaxParticleCount += EmitterInstance->MaxActiveParticles;
		}
		else delete EmitterInstance;
	}

	if (MaxParticleCount > 0)
	{
		SpriteBatcher = MakeUnique<FSpriteBatcher>();
		SpriteBatcher->Init(MaxParticleCount);		// PSC당 Batcher 하나 사용, 전체 파티클 세팅
	}
	UpdateBoundsExtent();	// Template이 정해졌으니 모듈 값으로 컬링 범위 다시 계산
}

void UParticleSystemComponent::ResetSystem()
{
	for (FParticleEmitterInstance* Inst : EmitterInstances)
	{
		delete Inst;
	}
	EmitterInstances.Empty();
	SpriteBatcher.reset();
}

void UParticleSystemComponent::TickComponent(float deltaTime)
{
	for (FParticleEmitterInstance* Inst : EmitterInstances)
	{
		Inst->Update(deltaTime);
		Inst->Spawn(deltaTime);
	}
}

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

		FRenderPacket RenderPac;

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
	const float Extent = BoundsExtent;
	return FBox{ FVector(-Extent, -Extent, -Extent), FVector(Extent, Extent, Extent) };
}

void UParticleSystemComponent::UpdateBoundsExtent()
{
	// 벡터의 세 축 중 절댓값이 가장 큰 값 (정육면체 박스 반경으로 사용)
	auto MaxAbsAxis = [](const FVector& V)
	{
		const FVector A = V.GetAbs();
		return std::max(A.X, std::max(A.Y, A.Z));
	};

	BoundsExtent = 0.0f;
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		const UParticleLODLevel* LODLevel = Instance->CurrentLODLevel;

		// 모듈이 없으면 0: Location/Velocity/Size 모듈이 없으면 그만큼 퍼지지 않음
		float OriginExtent = MaxAbsAxis(LODLevel->GetRequiredModule()->GetEmitterOrigin());
		float LocationExtent = 0.0f;
		float SpeedExtent = 0.0f;
		float LifeTime = 0.0f;
		float SizeExtent = 0.0f;

		for (UParticleModule* Module : LODLevel->GetModules())
		{
			if (const UParticleModuleLocation* Location = Cast<UParticleModuleLocation>(Module))
				LocationExtent = std::max(MaxAbsAxis(Location->GetMinLocation()), MaxAbsAxis(Location->GetMaxLocation()));
			else if (const UParticleModuleVelocity* Velocity = Cast<UParticleModuleVelocity>(Module))
				SpeedExtent = std::max(MaxAbsAxis(Velocity->GetStartVelocityMin()), MaxAbsAxis(Velocity->GetStartVelocityMax()));	
			else if (const UParticleModuleLifetime* Lifetime = Cast<UParticleModuleLifetime>(Module))
				LifeTime = Lifetime->GetMaxLifeTime();
			else if (const UParticleModuleSize* Size = Cast<UParticleModuleSize>(Module))
				SizeExtent = MaxAbsAxis(Size->GetMaxSize()) * 0.5f;
		}

		// 수명 동안 초기 속도로 이동한 거리까지 포함 (속도를 바꾸는 Update 모듈이 생기면 다시 검토)
		const float EmitterExtent = OriginExtent + LocationExtent + SpeedExtent * LifeTime + SizeExtent;
		BoundsExtent = std::max(BoundsExtent, EmitterExtent);
	}

	OnTransformDirty();
}
