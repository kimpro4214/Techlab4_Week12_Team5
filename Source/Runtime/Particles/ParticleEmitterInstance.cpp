#include "EnginePCH.h"
#include "ParticleEmitterInstance.h"
#include "Particles/ParticleEmitter.h"

#include <algorithm>
#include <cstring>
#include <span>
#include <type_traits>

static_assert(std::is_trivially_copyable_v<FBaseParticle>);
static_assert(std::is_trivially_copyable_v<FOrbitParticlePayload>);

void FParticleEmitterInstance::Init(const FParticleEmitter& Emitter)
{
	Reset();

	const auto AlignUp = [](uint32 Size, uint32 Alignment)
		{
			return ((Size + Alignment - 1) / Alignment) * Alignment;
		};

	static_assert(SlotAlignment % alignof(FBaseParticle) == 0);
	static_assert(SlotAlignment % alignof(FOrbitParticlePayload) == 0);

	uint32 SlotSize = sizeof(FBaseParticle);
	OrbitPayloadOffset = 0;

	if (Emitter.Orbit.bEnabled)
	{
		OrbitPayloadOffset = AlignUp(SlotSize, alignof(FOrbitParticlePayload));
		SlotSize = OrbitPayloadOffset + sizeof(FOrbitParticlePayload);
	}

	ParticleStride = AlignUp(SlotSize, SlotAlignment);
}

std::size_t FParticleEmitterInstance::GetSlotOffset(int32 ActiveIndex) const
{
	assert(ActiveIndex >= 0 && ActiveIndex < ActiveParticleCount);
	return static_cast<std::size_t>(ParticleIndices[ActiveIndex]) * ParticleStride;
}

int32 FParticleEmitterInstance::AddParticle(const FBaseParticle& Particle)
{
	if (ActiveParticleCount == ParticleIndices.Num())
	{
		const int32 NewSlot = ParticleIndices.Num();
		const auto SlotCount = static_cast<std::size_t>(NewSlot) + 1;
		ParticleData.resize(SlotCount * (ParticleStride / SlotAlignment));
		ParticleIndices.Add(NewSlot);
	}

	const int32 ActiveIndex = ActiveParticleCount++;
	SetParticle(ActiveIndex, Particle);
	if (HasOrbitPayload())
	{
		SetOrbitPayload(ActiveIndex, FOrbitParticlePayload{});
	}
	return ActiveIndex;
}

FBaseParticle FParticleEmitterInstance::GetParticle(int32 ActiveIndex) const
{
	FBaseParticle Particle{};
	const auto Bytes = std::as_bytes(std::span(ParticleData));
	std::memcpy(&Particle, Bytes.data() + GetSlotOffset(ActiveIndex), sizeof(Particle));
	return Particle;
}

void FParticleEmitterInstance::SetParticle(int32 ActiveIndex, const FBaseParticle& Particle)
{
	const auto Bytes = std::as_writable_bytes(std::span(ParticleData));
	std::memcpy(Bytes.data() + GetSlotOffset(ActiveIndex), &Particle, sizeof(Particle));
}

FOrbitParticlePayload FParticleEmitterInstance::GetOrbitPayload(int32 ActiveIndex) const
{
	assert(HasOrbitPayload());
	FOrbitParticlePayload Payload{};
	const auto Bytes = std::as_bytes(std::span(ParticleData));
	const auto Offset = GetSlotOffset(ActiveIndex) + OrbitPayloadOffset;
	std::memcpy(&Payload, Bytes.data() + Offset, sizeof(Payload));
	return Payload;
}

void FParticleEmitterInstance::SetOrbitPayload(int32 ActiveIndex, const FOrbitParticlePayload& Payload)
{
	assert(HasOrbitPayload());
	const auto Bytes = std::as_writable_bytes(std::span(ParticleData));
	const auto Offset = GetSlotOffset(ActiveIndex) + OrbitPayloadOffset;
	std::memcpy(Bytes.data() + Offset, &Payload, sizeof(Payload));
}

void FParticleEmitterInstance::Reset()
{
	ParticleData.clear();
	ParticleIndices.Reset();
	ActiveParticleCount = 0;
	SpawnRemainder = 0.0f;
	EmitterTime = 0.0f;
}

void FParticleEmitterInstance::UpdateParticles(float DeltaTime)
{
	for (int32 Index = ActiveParticleCount - 1; Index >= 0; --Index)
	{
		FBaseParticle Particle = GetParticle(Index);
		Particle.Location += Particle.Velocity * DeltaTime;
		Particle.Age += DeltaTime;

		if (Particle.Age >= Particle.LifeTime)
		{
			std::swap(ParticleIndices[Index], ParticleIndices[ActiveParticleCount - 1]);
			--ActiveParticleCount;
			continue;
		}
		SetParticle(Index, Particle);
	}
}

int32 FParticleEmitterInstance::ConsumeSpawnCount(const FParticleEmitter& Emitter, float DeltaTime)
{
	float SpawnDeltaTime = DeltaTime;
	if (Emitter.Required.Duration > 0.0f)
	{
		const float RemainingTime = Emitter.Required.Duration - EmitterTime;
		SpawnDeltaTime = std::min(DeltaTime, std::max(0.0f, RemainingTime));
	}

	EmitterTime += DeltaTime;
	return Emitter.Spawn.GetSpawnCount(SpawnDeltaTime, SpawnRemainder);
}
