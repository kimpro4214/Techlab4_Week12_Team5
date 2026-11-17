#pragma once

#include "Render/Particle/ParticleModuleTypeDataBase.h"
#include "Render/Particle/ParticleHelper.h"
#include "Render/Mesh.h"
#include "ParticleModuleTypeDataMesh.generated.h"

// 파티클 하나마다 스프라이트 대신 스태틱 메시를 그린다
// 메시는 자기 머티리얼(섹션별)로 그려진다
UCLASS()
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{
	GENERATED_BODY()
public:
	FParticleEmitterInstance* CreateInstance() override;

	UPROPERTY(EditAnywhere) UStaticMesh* Mesh = nullptr;

	// 메시 방향. None: 파티클 Rotation만큼 Z축으로 돈다 / Velocity: 메시 X축이 진행 방향을 본다
	EParticleMeshAlignment Alignment = EParticleMeshAlignment::None;
};
