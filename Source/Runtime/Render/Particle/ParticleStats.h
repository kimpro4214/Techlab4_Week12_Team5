#pragma once

#include "Core/Stats/LightweightStats.h"

// 파티클 CPU 시간. stat profile 목록에 자동으로 뜨고, stat particles에서도 따로 보여준다
namespace ParticleStats
{
	DECLARE_CYCLE_STAT("Particle Tick", STAT_ParticleTick);                  // 시뮬레이션 + Bounds
	DECLARE_CYCLE_STAT("Particle Render Data", STAT_ParticleRenderData);    // ReplayData·DynamicData 만들기
	DECLARE_CYCLE_STAT("Particle Submit", STAT_ParticleSubmit);             // 뷰마다 정점·패킷 만들기 (정렬 포함)
}
