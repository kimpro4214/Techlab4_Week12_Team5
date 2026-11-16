#include "EnginePCH.h"
#include "Render/Particle/ParticleSpriteEmitterInstance.h"
#include "Render/Particle/ParticleDynamicData.h"   // 렌더 헤더는 .cpp에서만 include한다
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"

FParticleSpriteEmitterInstance::FParticleSpriteEmitterInstance() = default;
FParticleSpriteEmitterInstance::~FParticleSpriteEmitterInstance() = default;

bool FParticleSpriteEmitterInstance::FillReplayData(FDynamicEmitterReplayDataBase& OutData)
{
	if (!FParticleEmitterInstance::FillReplayData(OutData))
		return false;

	FDynamicSpriteEmitterReplayDataBase& SpriteData = static_cast<FDynamicSpriteEmitterReplayDataBase&>(OutData);
	SpriteData.eEmitterType = EDynamicEmitterType::Sprite;
	SpriteData.Material     = CurrentLODLevel->GetRequiredModule()->GetMaterial();
	SpriteData.MaxParticles = MaxActiveParticles;
	return true;
}

FDynamicEmitterDataBase* FParticleSpriteEmitterInstance::GetDynamicData()
{
	if (!CurrentLODLevel)
		return nullptr;

	if (!DynamicData)
		DynamicData = MakeUnique<FDynamicSpriteEmitterData>();

	// 살아있는 파티클이 없으면 그릴 것도 없다
	if (!FillReplayData(DynamicData->Source))
		return nullptr;

	// 버퍼 용량 확인 + 뷰 인덱스 리셋. 뷰 인덱스를 되돌리므로 프레임당 한 번만 불러야 한다
	DynamicData->Init(MaxActiveParticles);
	return DynamicData.get();
}
