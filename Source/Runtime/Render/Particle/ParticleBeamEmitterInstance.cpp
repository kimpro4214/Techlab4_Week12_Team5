#include "EnginePCH.h"
#include "Render/Particle/ParticleBeamEmitterInstance.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleDynamicData.h"   // 렌더 헤더는 .cpp에서만 include한다
#include "Render/Particle/ParticleModuleRequired.h"
#include <algorithm>

FParticleBeamEmitterInstance::FParticleBeamEmitterInstance() = default;
FParticleBeamEmitterInstance::~FParticleBeamEmitterInstance() = default;

bool FParticleBeamEmitterInstance::Init(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent)
{
	if (!FParticleEmitterInstance::Init(InTemplate, InComponent))
		return false;

	// TODO: TypeData 기반이 들어오면 여기서 UParticleModuleTypeDataBeam의 TargetOffset / SegmentCount / NoiseAmplitude를 복사한다

	SegmentCount = std::max(SegmentCount, 1);

	MaxPoints = MaxActiveParticles * (SegmentCount + 1);
	return true;
}

bool FParticleBeamEmitterInstance::FillReplayData(FDynamicEmitterReplayDataBase& OutData)
{
	if (!FParticleEmitterInstance::FillReplayData(OutData))
		return false;

	FDynamicStripEmitterReplayDataBase& BeamData = static_cast<FDynamicStripEmitterReplayDataBase&>(OutData);
	BeamData.eEmitterType = EDynamicEmitterType::Beam;
	BeamData.Material     = CurrentLODLevel->GetRequiredModule()->GetMaterial();
	BeamData.MaxPoints    = MaxPoints;

	BeamData.Points.Reset();
	BeamData.StripPointCounts.Reset();

	FVector Source = EmitterToSimulation.GetOrigin();
	FVector Target = EmitterToSimulation.TransformPosition(TargetOffset);

	for (int32 i = 0; i < ActiveParticles; i++)
	{
		const uint16 Slot = ParticleDataBlock.ParticleIndices[i];
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			ParticleDataBlock.ParticleData + Slot * ParticleStride);

		for (int32 j = 0; j <= SegmentCount; j++)
		{
			const float Alpha = static_cast<float>(j) / static_cast<float>(SegmentCount);
			FVector Pos = FMath::Lerp(Source, Target, Alpha);

			// 양 끝(Source, Target)은 고정하고 중간 점만 랜덤 Noise 부여
			if (NoiseAmplitude > 0.0f && j > 0 && j < SegmentCount)
				Pos += ParticleRandRange(FVector(-1.0f, -1.0f, -1.0f), FVector(1.0f, 1.0f, 1.0f)) * NoiseAmplitude;

			BeamData.Points.Add({ Pos, Particle.Size.X, Particle.Color });
		}
		BeamData.StripPointCounts.Add(SegmentCount + 1);
	}
	return true;
}

FDynamicEmitterDataBase* FParticleBeamEmitterInstance::GetDynamicData()
{
	if (!CurrentLODLevel)
		return nullptr;

	if (!DynamicData)
		DynamicData = MakeUnique<FDynamicStripEmitterData>();

	// 살아있는 빔이 없으면 그릴 것도 없다
	if (!FillReplayData(DynamicData->Source))
		return nullptr;

	// 버퍼 용량 확인 + 뷰 인덱스 리셋. 뷰 인덱스를 되돌리므로 프레임당 한 번만 불러야 한다
	DynamicData->Init(DynamicData->Source.MaxPoints);
	return DynamicData.get();
}
