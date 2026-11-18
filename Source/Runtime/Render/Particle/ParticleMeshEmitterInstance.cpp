#include "EnginePCH.h"
#include "Render/Particle/ParticleMeshEmitterInstance.h"
#include "Render/Particle/ParticleDynamicData.h"   // 렌더 헤더는 .cpp에서만 include한다
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleTypeDataMesh.h"
#include <algorithm>
#include <cmath>

FParticleMeshEmitterInstance::FParticleMeshEmitterInstance(UParticleModuleTypeDataMesh* InMeshTypeData)
	: MeshTypeData(InMeshTypeData)
{
}

FParticleMeshEmitterInstance::~FParticleMeshEmitterInstance() = default;

float FParticleMeshEmitterInstance::GetParticleExtent(const FBaseParticle& Particle) const
{
	if (!MeshTypeData || !MeshTypeData->Mesh)
		return FParticleEmitterInstance::GetParticleExtent(Particle);

	// 메시 원점에서 가장 먼 모서리까지 거리. 어느 방향으로 돌아도 이 반지름 안에 들어간다
	const FBox& MeshBounds = MeshTypeData->Mesh->GetRenderBounds();
	const FVector FarCorner(
		std::max(std::abs(MeshBounds.Min.X), std::abs(MeshBounds.Max.X)),
		std::max(std::abs(MeshBounds.Min.Y), std::abs(MeshBounds.Max.Y)),
		std::max(std::abs(MeshBounds.Min.Z), std::abs(MeshBounds.Max.Z)));
	const float MeshRadius = std::sqrt(FarCorner.Dot(FarCorner));

	const float MaxSize = std::max({ std::abs(Particle.Size.X), std::abs(Particle.Size.Y), std::abs(Particle.Size.Z) });
	return MeshRadius * MaxSize;
}

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
	MeshData.Material     = CurrentLODLevel->GetRequiredModule()->GetMaterial();
	MeshData.MaxParticles = MaxActiveParticles;

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

	// 인스턴스 행렬은 뷰와 상관없으므로 프레임당 한 번만 만들어 올린다
	DynamicData->Init();
	return DynamicData.get();
}
