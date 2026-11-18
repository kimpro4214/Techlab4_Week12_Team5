#include "EnginePCH.h"
#include "Render/Particle/ParticleRibbonEmitterInstance.h"
#include "Render/Particle/ParticleDynamicData.h"   // 렌더 헤더는 .cpp에서만 include한다
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleTypeDataRibbon.h"
#include <algorithm>

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

	// 점이 2개 미만이면 띠가 생기지 않는다
	if (SortedSlots.Num() < 2)
		return false;

	FDynamicRibbonReplayData& RibbonData = static_cast<FDynamicRibbonReplayData&>(OutData);
	RibbonData.eEmitterType = EDynamicEmitterType::Ribbon;
	RibbonData.Material     = CurrentLODLevel->GetRequiredModule()->GetMaterial();
	RibbonData.UVTiling     = RibbonTypeData ? RibbonTypeData->UVTiling : 0.0f;

	// 정렬된 순서대로 파티클을 점으로 옮긴다. 렌더 쪽(띠 빌더)은 월드 좌표 점 목록만 받는다
	// Reset은 용량을 유지하므로 매 프레임 재할당하지 않는다
	RibbonData.Points.Reset();
	for (int32 Slot : SortedSlots)
	{
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			ParticleDataBlock.ParticleData + Slot * ParticleStride);

		FParticleStripPoint Point;
		Point.Position = SimulationToWorld.TransformPosition(Particle.Location);   // 시뮬레이션 공간 → 월드
		Point.Width    = Particle.Size.X;
		Point.Color    = Particle.Color;
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
	return DynamicData.get();
}
