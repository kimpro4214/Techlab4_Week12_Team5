#include "EnginePCH.h"
#include "Render/Particle/ParticleBeamEmitterInstance.h"
#include "Render/Particle/ParticleDynamicData.h"   // 렌더 헤더는 .cpp에서만 include한다
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleTypeDataBeam.h"
#include "Components/ParticleSystemComponent.h"
#include <algorithm>
#include <cmath>

FParticleBeamEmitterInstance::FParticleBeamEmitterInstance(UParticleModuleTypeDataBeam* InBeamTypeData)
	: BeamTypeData(InBeamTypeData)
{
}

FParticleBeamEmitterInstance::~FParticleBeamEmitterInstance() = default;

void FParticleBeamEmitterInstance::GetEndpoints(FVector& OutSource, FVector& OutTarget) const
{
	OutSource = SimulationToWorld.TransformPosition(EmitterToSimulation.GetOrigin());
	OutTarget = SimulationToWorld.TransformPosition(EmitterToSimulation.TransformPosition(BeamTypeData->TargetOffset));
}

bool FParticleBeamEmitterInstance::GetLocalBoundingBox(const FMatrix& WorldToComponent, FBox& OutBox) const
{
	if (!BeamTypeData)
		return false;

	// 두 끝점을 감싸고, 노이즈로 휘는 만큼은 패딩으로 더한다
	FVector Source, Target;
	GetEndpoints(Source, Target);
	const FVector A = WorldToComponent.TransformPosition(Source);
	const FVector B = WorldToComponent.TransformPosition(Target);

	// 폭은 월드 단위라 컴포넌트 로컬로 옮기면 스케일만큼 나눠야 한다. 가장 작은 축 기준으로 넉넉하게
	const FVector Scale = Component->GetWorldScale3D();
	const float MinScale = std::max(1e-4f, std::min({ std::abs(Scale.X), std::abs(Scale.Y), std::abs(Scale.Z) }));
	// 노이즈로 중간 점이 각 축으로 최대 NoiseAmplitude만큼 벗어나므로 그만큼 더 넓힌다
	const float Pad = (BeamTypeData->Width * 0.5f + std::max(0.0f, BeamTypeData->NoiseAmplitude)) / MinScale;

	OutBox = FBox(
		FVector(std::min(A.X, B.X) - Pad, std::min(A.Y, B.Y) - Pad, std::min(A.Z, B.Z) - Pad),
		FVector(std::max(A.X, B.X) + Pad, std::max(A.Y, B.Y) + Pad, std::max(A.Z, B.Z) + Pad));
	return true;
}

bool FParticleBeamEmitterInstance::FillReplayData(FDynamicEmitterReplayDataBase& OutData)
{
	if (!BeamTypeData)
		return false;

	// Beam은 파티클 수와 상관없이 그린다 → 부모(활성 파티클이 없으면 실패)를 거치지 않고 필요한 값만 채운다
	FDynamicBeamReplayData& BeamData = static_cast<FDynamicBeamReplayData&>(OutData);
	BeamData.eEmitterType       = EDynamicEmitterType::Beam;
	BeamData.SimulationToWorld  = SimulationToWorld;
	BeamData.EmitterWorldOrigin = SimulationToWorld.TransformPosition(EmitterToSimulation.GetOrigin());
	BeamData.Material           = CurrentLODLevel->GetRequiredModule()->GetMaterial();
	BeamData.UVTiling           = BeamTypeData->UVTiling;

	// Source → Target을 SegmentCount개 구간으로 나누고 중간 점에 노이즈를 더한다
	FVector Source, Target;
	GetEndpoints(Source, Target);
	const int32 Segments = std::max(1, BeamTypeData->SegmentCount);

	BeamData.Points.SetNum(Segments + 1, false);
	for (int32 i = 0; i <= Segments; ++i)
	{
		const float Alpha = static_cast<float>(i) / static_cast<float>(Segments);
		FParticleStripPoint& Point = BeamData.Points[i];
		Point.Position = Source + (Target - Source) * Alpha;

		// 양 끝(Source, Target)은 고정하고 중간 점만 3D 랜덤으로 흔든다
		// 매 프레임 새 난수라 빔 모양이 프레임마다 바뀐다 (전기가 지글거리는 효과)
		// NoiseAmplitude가 구간 길이(빔 길이 / SegmentCount)보다 크면 점이 앞뒤로 뒤집혀 띠가 접힐 수 있다
		if (BeamTypeData->NoiseAmplitude > 0.0f && i > 0 && i < Segments)
			Point.Position += ParticleRandRange(FVector(-1.0f, -1.0f, -1.0f), FVector(1.0f, 1.0f, 1.0f)) * BeamTypeData->NoiseAmplitude;

		Point.Width    = BeamTypeData->Width;
		Point.Color    = BeamTypeData->Color;
	}
	return true;
}

FDynamicEmitterDataBase* FParticleBeamEmitterInstance::GetDynamicData()
{
	if (!CurrentLODLevel)
		return nullptr;

	if (!DynamicData)
		DynamicData = MakeUnique<FDynamicBeamEmitterData>();

	if (!FillReplayData(DynamicData->Source))
		return nullptr;

	// 뷰 버퍼 사용 위치를 되돌린다. 프레임당 한 번만 불러야 한다
	DynamicData->Init();
	return DynamicData.get();
}
