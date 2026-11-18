#include "EnginePCH.h"
#include "Render/Particle/ParticleBeamEmitterInstance.h"
#include "Render/Particle/ParticleDynamicData.h"   // 렌더 헤더는 .cpp에서만 include한다
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleTypeDataBeam.h"
#include <algorithm>

FParticleBeamEmitterInstance::FParticleBeamEmitterInstance(UParticleModuleTypeDataBeam* InBeamTypeData)
	: BeamTypeData(InBeamTypeData)
{
}

FParticleBeamEmitterInstance::~FParticleBeamEmitterInstance() = default;

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

	// [렌더링 확인용] 직선 Source → Target을 SegmentCount개 구간으로 나눈다
	// 실제 점 생성(타깃 지정, 노이즈, 수명)은 시뮬레이션 담당이 이 배열을 채우는 코드로 바꾼다
	const FVector Source = BeamData.EmitterWorldOrigin;
	const FVector Target = SimulationToWorld.TransformPosition(EmitterToSimulation.TransformPosition(BeamTypeData->TargetOffset));
	const int32 Segments = std::max(1, BeamTypeData->SegmentCount);

	BeamData.Points.SetNum(Segments + 1, false);
	for (int32 i = 0; i <= Segments; ++i)
	{
		const float Alpha = static_cast<float>(i) / static_cast<float>(Segments);
		FParticleStripPoint& Point = BeamData.Points[i];
		Point.Position = Source + (Target - Source) * Alpha;
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
