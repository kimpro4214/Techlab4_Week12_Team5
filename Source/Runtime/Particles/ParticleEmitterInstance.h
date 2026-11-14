#pragma once

#include "Containers/Array.h"
#include "Math/Vector.h"

#include <cstddef>
#include <vector>

struct FParticleEmitter;
struct FOrbitParticlePayload;

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

	int32 GetActiveParticleCount() const { return ActiveParticleCount; }
	int32 AddParticle(const FBaseParticle& Particle);

	FBaseParticle GetParticle(int32 ActiveIndex) const;
	void SetParticle(int32 ActiveIndex, const FBaseParticle& Particle);

	bool HasOrbitPayload() const { return OrbitPayloadOffset != 0; }
	FOrbitParticlePayload GetOrbitPayload(int32 ActiveIndex) const;
	void SetOrbitPayload(int32 ActiveIndex, const FOrbitParticlePayload& Payload);


private:
	float SpawnRemainder = 0.0f;
	float EmitterTime = 0.0f;
	uint32 OrbitPayloadOffset = 0;

	static constexpr uint32 SlotAlignment = 16;

	struct alignas(SlotAlignment) FParticleStorageBlock
	{
		std::byte Bytes[SlotAlignment];
	};

	static_assert(sizeof(FParticleStorageBlock) == SlotAlignment);

	std::size_t GetSlotOffset(int32 ActiveIndex) const;

	std::vector<FParticleStorageBlock> ParticleData;
	TArray<int32> ParticleIndices;
	int32 ActiveParticleCount = 0;
	uint32 ParticleStride = ((sizeof(FBaseParticle) + SlotAlignment - 1) / SlotAlignment) * SlotAlignment;
};
