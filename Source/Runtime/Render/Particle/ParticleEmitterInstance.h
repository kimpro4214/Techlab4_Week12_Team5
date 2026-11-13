#pragma once

#include "EnginePCH.h"
#include "ParticleHelper.h"

// Component × Emitter마다 1개씩 존재하는 Emitter의 실행 상태
// 파티클 메모리를 보유하고 Spawn / Update / Kill을 수행
struct FParticleEmitterInstance
{
};
