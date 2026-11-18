#include "EnginePCH.h"
#include "Render/Particle/ParticleRibbonEmitterInstance.h"
#include "Render/Particle/ParticleDynamicData.h"   // 렌더 헤더는 .cpp에서만 include한다
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleTypeDataRibbon.h"
#include <algorithm>
#include <cmath>

FParticleRibbonEmitterInstance::FParticleRibbonEmitterInstance(UParticleModuleTypeDataRibbon* InRibbonTypeData)
	: RibbonTypeData(InRibbonTypeData)
{
}

FParticleRibbonEmitterInstance::~FParticleRibbonEmitterInstance() = default;

bool FParticleRibbonEmitterInstance::Init(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent)
{
	if (!FParticleEmitterInstance::Init(InTemplate, InComponent))
		return false;

	// MaxActiveParticles는 부모 Init이 정하므로 그 뒤에 예약한다
	SortedSlots.Reserve(MaxActiveParticles);

	// 칸마다 마디의 탄생 거리. 아직 아무것도 기록하지 않은 칸은 쓰이지 않는 Id로 표시한다
	SlotParticleIds.SetNum(MaxActiveParticles);
	SlotBirthDistances.SetNum(MaxActiveParticles);
	SlotBirthTimes.SetNum(MaxActiveParticles);
	SlotBirthTangents.SetNum(MaxActiveParticles);
	for (int32 i = 0; i < MaxActiveParticles; ++i)
	{
		SlotParticleIds[i]    = ~0u;
		SlotBirthDistances[i] = 0.0f;
		SlotBirthTimes[i]     = 0.0f;
		SlotBirthTangents[i]  = FVector::ZeroVector;
	}
	bHasHead = false;
	return true;
}

bool FParticleRibbonEmitterInstance::FillReplayData(FDynamicEmitterReplayDataBase& OutData)
{
	// 리본은 파티클이 곧 점이라, 살아있는 파티클이 없으면(부모 실패) 그릴 것도 없다
	if (!FParticleEmitterInstance::FillReplayData(OutData))
		return false;

	SortedSlots.Reset();
	for (int32 i = 0; i < ActiveParticles; ++i)
		SortedSlots.Add(ParticleDataBlock.ParticleIndices[i]);

	// ParticleId는 생성 시 ParticleCounter++로 붙는 번호라 클수록 최신이다 → 내림차순 = 머리(최신) → 꼬리
	// 칸이 재사용되면 PreSpawn 후 새 Id로 덮어써지므로 따로 관리하는 배열과 어긋날 일이 없다
	const auto GetParticleId = [this](int32 Slot)
	{
		return reinterpret_cast<const FBaseParticle*>(ParticleDataBlock.ParticleData + Slot * ParticleStride)->ParticleId;
	};
	std::sort(SortedSlots.begin(), SortedSlots.end(),
		[&GetParticleId](int32 A, int32 B) { return GetParticleId(A) > GetParticleId(B); });

	// 새로 태어난 마디에 누적 거리를 기록한다
	// 오래된 것부터(Id 오름차순 = 배열 뒤에서부터) 처리해야 같은 프레임에 여러 개 태어나도 거리가 순서대로 쌓인다
	// 한 번 기록한 거리는 바꾸지 않는다 → 머리가 움직이고 꼬리가 죽어도 마디의 U가 그대로라 무늬가 흐르지 않는다
	for (int32 i = SortedSlots.Num() - 1; i >= 0; --i)
	{
		const int32 Slot = SortedSlots[i];
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			ParticleDataBlock.ParticleData + Slot * ParticleStride);
		if (SlotParticleIds[Slot] == Particle.ParticleId)
			continue;   // 이미 기록한 마디

		const FVector WorldPosition = SimulationToWorld.TransformPosition(Particle.Location);
		float   Distance = 0.0f;
		FVector Tangent  = FVector::ZeroVector;
		if (bHasHead)
		{
			const FVector Delta = WorldPosition - LastHeadPosition;
			Distance = LastHeadDistance + std::sqrt(Delta.Dot(Delta));

			// 속도 접선 = 직전 마디에서 여기까지 움직인 거리 ÷ 걸린 시간
			// 같은 프레임에 여러 개 태어나면 시간 차가 0이라 직전 마디의 접선을 이어 쓴다
			const float TimeDelta = EmitterTime - LastHeadTime;
			Tangent = TimeDelta > 1e-5f ? Delta / TimeDelta : LastHeadTangent;
		}

		SlotParticleIds[Slot]    = Particle.ParticleId;
		SlotBirthDistances[Slot] = Distance;
		SlotBirthTimes[Slot]     = EmitterTime;
		SlotBirthTangents[Slot]  = Tangent;
		bHasHead         = true;
		LastHeadPosition = WorldPosition;
		LastHeadDistance = Distance;
		LastHeadTime     = EmitterTime;
		LastHeadTangent  = Tangent;
	}

	// 점이 2개 미만이면 띠가 생기지 않는다
	if (SortedSlots.Num() < 2)
		return false;

	FDynamicRibbonReplayData& RibbonData = static_cast<FDynamicRibbonReplayData&>(OutData);
	RibbonData.eEmitterType = EDynamicEmitterType::Ribbon;
	RibbonData.Material     = CurrentLODLevel->GetRequiredModule()->GetMaterial();
	RibbonData.UVTiling     = RibbonTypeData ? RibbonTypeData->UVTiling : 0.0f;
	RibbonData.DistanceTessellationStepSize    = RibbonTypeData ? RibbonTypeData->DistanceTessellationStepSize : 0.0f;
	RibbonData.TangentTessellationScalar       = RibbonTypeData ? RibbonTypeData->TangentTessellationScalar : 0.0f;
	RibbonData.MaxTessellationBetweenParticles = RibbonTypeData ? RibbonTypeData->MaxTessellationBetweenParticles : 0;
	RibbonData.bTangentRecalculationEveryFrame = RibbonTypeData ? RibbonTypeData->bTangentRecalculationEveryFrame : false;

	// 정렬된 순서대로 파티클을 점으로 옮긴다. 렌더 쪽(띠 빌더)은 월드 좌표 점 목록만 받는다
	// Reset은 용량을 유지하므로 매 프레임 재할당하지 않는다
	RibbonData.Points.Reset();
	for (int32 Slot : SortedSlots)
	{
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			ParticleDataBlock.ParticleData + Slot * ParticleStride);

		FParticleStripPoint Point;
		Point.Position = SimulationToWorld.TransformPosition(Particle.Location);   // 시뮬레이션 공간 → 월드
		// 위치는 SimulationToWorld로 스케일이 들어가지만 폭은 월드에서 펼치므로 스프라이트처럼 스케일을 따로 곱한다
		Point.Width    = std::abs(Particle.Size.X * RibbonData.Scale.X);
		Point.Color    = Particle.Color;
		Point.Distance = SlotBirthDistances[Slot];
		Point.Tangent  = SlotBirthTangents[Slot];
		Point.Time     = SlotBirthTimes[Slot];
		RibbonData.Points.Add(Point);
	}
	return true;
}

FDynamicEmitterDataBase* FParticleRibbonEmitterInstance::GetDynamicData()
{
	if (!CurrentLODLevel)
		return nullptr;

	if (!DynamicData)
		DynamicData = MakeUnique<FDynamicRibbonEmitterData>();

	if (!FillReplayData(DynamicData->Source))
		return nullptr;

	// 뷰 버퍼 사용 위치를 되돌린다. 프레임당 한 번만 불러야 한다
	DynamicData->Init();
	// 마디 사이 곡선 보간. 카메라와 상관없으므로 뷰마다 하지 않고 여기서 한 번만 한다
	DynamicData->Tessellate();
	return DynamicData.get();
}
