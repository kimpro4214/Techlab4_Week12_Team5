#pragma once

#include "Core/Types.h"
#include "Math/Vector.h"

class UParticleEmitter;
class UParticleSystemComponent;
class UParticleLODLevel;
struct FBaseParticle;
struct FParticleDataContainer;
struct FParticleEventInstancePayload;

struct FParticleEmitterInstance
{
public:
	FParticleEmitterInstance();
	~FParticleEmitterInstance();

	FParticleEmitterInstance(const FParticleEmitterInstance&) = delete;
	FParticleEmitterInstance& operator=(const FParticleEmitterInstance&) = delete;

	void Initialize(
		UParticleEmitter* InSpriteTemplate,
		UParticleSystemComponent* InComponent,
		int32 InCurrentLODLevelIndex,
		UParticleLODLevel* InCurrentLODLevel,
		int32 InCapacity = 1024,
		float InDefaultLifetime = 1.0f
	);

	// StartTime은 프레임 끝 기준 첫 입자의 경과 시간(초), Increment는 생성 간격(초)입니다.
	void SpawnParticles(
		int32 Count,
		float StartTime,
		float Increment,
		const FVector& InitialLocation,
		const FVector& InitialVelocity,
		FParticleEventInstancePayload* EventPayload = nullptr
	);

	// Index는 실제 슬롯 번호가 아닌 활성 목록의 순번입니다.
	void KillParticle(int32 Index);
	// RelativeTime이 1 이상인 입자를 제거합니다. 수명 갱신은 호출자가 수행합니다.
	void KillParticles();

	int32 GetActiveParticleCount() const { return ActiveParticles; }
	int32 GetCapacity() const { return MaxActiveParticles; }
	int32 GetParticleStride() const { return ParticleStride; }
	uint32 GetParticleCounter() const { return ParticleCounter; }

	FBaseParticle* GetParticle(int32 Index);
	const FBaseParticle* GetParticle(int32 Index) const;
	int32 GetParticleSlot(int32 Index) const;

private:
	void InitializeParticleStorage(int32 InParticleStride, int32 InCapacity);

	UParticleEmitter* SpriteTemplate = nullptr;
	UParticleSystemComponent* Component = nullptr;

	TUniquePtr<FParticleDataContainer> Container;
	int32 CurrentLODLevelIndex = 0;
	UParticleLODLevel* CurrentLODLevel = nullptr;

	// Container가 소유하는 메모리를 가리키는 비소유 포인터입니다.
	uint8* ParticleData = nullptr;
	uint16* ParticleIndices = nullptr;

	// 모듈 Payload는 후속 단계에서 배치합니다.
	uint8* InstanceData = nullptr;
	int32 InstancePayloadSize = 0;
	int32 PayloadOffset = 0;

	// 입자 하나의 Payload 포함 크기와 슬롯 사이의 바이트 간격입니다.
	int32 ParticleSize = 0;
	int32 ParticleStride = 0;
	int32 ActiveParticles = 0;
	uint32 ParticleCounter = 0;
	int32 MaxActiveParticles = 0;
	float DefaultLifetime = 1.0f;
};
