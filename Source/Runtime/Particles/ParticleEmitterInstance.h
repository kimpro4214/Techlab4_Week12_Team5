#pragma once

#include "Containers/Array.h"
#include "Math/Vector.h"

struct FParticle
{
	// 파티클 한 개의 물리·수명·SubUV 프레임 상태를 담는다.
	FVector Location;
	FVector Velocity;
	float LifeTime;

	float Age;
	float Scale;
	uint32 SubUVFrame;
	float FrameTimer;

	float Alpha;

	bool bAlive;
};

class FParticleEmitterInstance
{
public:
	void Reset();
	void UpdateParticles(float DeltaTime);
	int32 ConsumeSpawnCount(float SpawnRate, float DeltaTime);

	TArray<FParticle>& GetParticles() { return Particles; }
	const TArray<FParticle>& GetParticles() const { return Particles; }


private:
	TArray<FParticle> Particles;
	float SpawnRemainder = 0.0f;
};
