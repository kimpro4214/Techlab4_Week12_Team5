#pragma once

#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleTypeDataBase.generated.h"

struct FParticleEmitterInstance;

// 이미터 종류를 정하는 모듈 (Mesh, Beam, Ribbon …)
// Modules 목록이 아니라 LODLevel의 TypeDataModule 칸에 들어간다. 비어 있으면 Sprite 이미터다
UCLASS()
class UParticleModuleTypeDataBase : public UParticleModule
{
	GENERATED_BODY()
public:
	// 이 종류에 맞는 EmitterInstance를 만든다. Init은 호출하는 쪽(UParticleEmitter)이 한다
	virtual FParticleEmitterInstance* CreateInstance() { return nullptr; }
};
