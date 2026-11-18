#include "EnginePCH.h"
#include "Render/Particle/ParticleRibbonEmitterInstance.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleDynamicData.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include <algorithm>

FParticleRibbonEmitterInstance::FParticleRibbonEmitterInstance() = default;
FParticleRibbonEmitterInstance::~FParticleRibbonEmitterInstance() = default;

bool FParticleRibbonEmitterInstance::FillReplayData(FDynamicEmitterReplayDataBase& OutData)
{
	if (!FParticleEmitterInstance::FillReplayData(OutData))
		return false;

	SortedSlots.Reset();
	for (int32 i = 0; i < ActiveParticles; ++i)
		SortedSlots.Add(ParticleDataBlock.ParticleIndices[i]);		// SortedSlot에 일단 ParticleIndices를 다 담음

	// ParticleId는 생성 시 ParticleCounter++로 붙는 번호라 클수록 최신이다 → 내림차순 = 머리(최신) → 꼬리
	// 칸이 재사용되면 PreSpawn 후 새 Id로 덮어써지므로 따로 관리하는 배열과 어긋날 일이 없다
	const auto GetParticleId = [this](int32 Slot)
	{
		return reinterpret_cast<const FBaseParticle*>(ParticleDataBlock.ParticleData + Slot * ParticleStride)->ParticleId;
	};
	std::sort(SortedSlots.begin(), SortedSlots.end(),
		[&GetParticleId](int32 A, int32 B) { return GetParticleId(A) > GetParticleId(B); });

	if (SortedSlots.Num() < 2)
		return false;

	FDynamicStripEmitterReplayDataBase& RibbonData = static_cast<FDynamicStripEmitterReplayDataBase&>(OutData);
	RibbonData.eEmitterType = EDynamicEmitterType::Ribbon;
	RibbonData.Material     = CurrentLODLevel->GetRequiredModule()->GetMaterial();
	RibbonData.MaxPoints    = MaxActiveParticles;   // 리본은 파티클 1개 = 점 1개

	// 정렬된 순서(머리 → 꼬리)대로 파티클을 점으로 옮긴다. 렌더 쪽은 파티클 메모리 구조를 몰라도 된다
	// Reset은 용량을 유지하므로 매 프레임 재할당하지 않는다
	RibbonData.Points.Reset();
	for (int32 Slot : SortedSlots)
	{
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			ParticleDataBlock.ParticleData + Slot * ParticleStride);		// SortedSlot에 정렬한 순서대로 Particle 읽음

		FParticleStripPoint Point;
		Point.Position = Particle.Location;  
		Point.Width    = Particle.Size.X;
		Point.Color    = Particle.Color;
		RibbonData.Points.Add(Point);
	}

	RibbonData.StripPointCounts.Reset();
	RibbonData.StripPointCounts.Add(RibbonData.Points.Num());
	return true;
}

FDynamicEmitterDataBase* FParticleRibbonEmitterInstance::GetDynamicData()
{
	if (!CurrentLODLevel)
		return nullptr;

	if (!DynamicData)
		DynamicData = MakeUnique<FDynamicStripEmitterData>();

	// 살아있는 파티클이 없으면 그릴 것도 없다
	if (!FillReplayData(DynamicData->Source))
		return nullptr;

	// 버퍼 용량 확인 + 뷰 인덱스 리셋. 뷰 인덱스를 되돌리므로 프레임당 한 번만 불러야 한다
	DynamicData->Init(DynamicData->Source.MaxPoints);
	return DynamicData.get();
}