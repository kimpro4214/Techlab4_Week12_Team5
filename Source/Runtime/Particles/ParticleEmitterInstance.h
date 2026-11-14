#pragma once

#include "Containers/Array.h"
#include "Math/Vector.h"

struct FParticleEmitter;

struct FBaseParticle
{
	// 파티클 한 개의 물리·수명·SubUV 프레임 상태를 담는다.
	FVector Location;
	FVector Velocity;
	float LifeTime;

	float Age;
	float Scale;
	uint32 SubUVFrame;
	float FrameTimer;

	FVector Color = FVector(1.0f, 1.0f, 1.0f);
	float Alpha;

	bool bAlive;
};

class FParticleEmitterInstance
{
public:
	void Init(const FParticleEmitter& Emitter);
	void Reset();
	void UpdateParticles(float DeltaTime);
	int32 ConsumeSpawnCount(const FParticleEmitter& Emitter, float DeltaTime);

	TArray<FBaseParticle>& GetParticles() { return Particles; }
	const TArray<FBaseParticle>& GetParticles() const { return Particles; }


private:
	TArray<FBaseParticle> Particles;
	float SpawnRemainder = 0.0f;
	float EmitterTime = 0.0f;
	uint32 ParticleStride = 0;
	uint32 OrbitPayloadOffset = 0;
};
