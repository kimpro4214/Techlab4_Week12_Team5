#include "EnginePCH.h"
#include "Render/Particle/ParticleMeshEmitterInstance.h"
#include "Render/Particle/ParticleDynamicData.h"   // 렌더 헤더는 .cpp에서만 include한다
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleTypeDataMesh.h"

FParticleMeshEmitterInstance::FParticleMeshEmitterInstance(UParticleModuleTypeDataMesh* InMeshTypeData)
	: MeshTypeData(InMeshTypeData)
{
}

FParticleMeshEmitterInstance::~FParticleMeshEmitterInstance() = default;

bool FParticleMeshEmitterInstance::FillReplayData(FDynamicEmitterReplayDataBase& OutData)
{
	if (!MeshTypeData || !MeshTypeData->Mesh)
		return false;   // 메시가 없으면 그릴 것이 없다

	if (!FParticleEmitterInstance::FillReplayData(OutData))
		return false;

	FDynamicMeshEmitterReplayData& MeshData = static_cast<FDynamicMeshEmitterReplayData&>(OutData);
	MeshData.eEmitterType = EDynamicEmitterType::Mesh;
	MeshData.Mesh         = MeshTypeData->Mesh;
	MeshData.Alignment    = MeshTypeData->Alignment;

	// 로컬 공간이면 SimulationToWorld(=컴포넌트 행렬)에 이미 스케일이 들어 있다. 두 번 곱하지 않도록 1로 둔다
	if (CurrentLODLevel->GetRequiredModule()->IsUseLocalSpace())
		MeshData.Scale = FVector(1.0f, 1.0f, 1.0f);
	return true;
}

FDynamicEmitterDataBase* FParticleMeshEmitterInstance::GetDynamicData()
{
	if (!CurrentLODLevel)
		return nullptr;

	if (!DynamicData)
		DynamicData = MakeUnique<FDynamicMeshEmitterData>();

	if (!FillReplayData(DynamicData->Source))
		return nullptr;
	return DynamicData.get();
}
