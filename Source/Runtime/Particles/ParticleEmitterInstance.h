#pragma once

#include "Core/Types.h"
#include "Math/Vector.h"
#include "ParticleHelper.h"

class UParticleEmitter;
class UParticleSystemComponent;
class UParticleLODLevel;
class UParticleModule;
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

	// Required 모듈의 용량을 사용하는 기본 설정 경로입니다.
	void Initialize(
		UParticleEmitter* InSpriteTemplate,
		UParticleSystemComponent* InComponent,
		int32 InCurrentLODLevelIndex,
		UParticleLODLevel* InCurrentLODLevel
	);

	// 용량을 명시하는 기존 경로이며 Required 설정보다 인자가 우선합니다.
	void Initialize(
		UParticleEmitter* InSpriteTemplate,
		UParticleSystemComponent* InComponent,
		int32 InCurrentLODLevelIndex,
		UParticleLODLevel* InCurrentLODLevel,
		int32 InCapacity,
		float InDefaultLifetime = 1.0f
	);

	// InitialLocation/Velocity는 월드 값입니다. 선택적 행렬은 모듈 초기값에만 적용합니다.
	// StartTime은 프레임 끝 기준 첫 입자의 경과 시간(초), Increment는 생성 간격(초)입니다.
	void SpawnParticles(
		int32 Count,
		float StartTime,
		float Increment,
		const FVector& InitialLocation,
		const FVector& InitialVelocity,
		FParticleEventInstancePayload* EventPayload = nullptr,
		const FMatrix* EmitterToWorld = nullptr
	);

	// 파티클을 정리합니다. Index는 ParticleIndices의 인덱스입니다.
	void KillParticle(int32 Index);

	// 수명이 끝난 파티클을 정리합니다. 수명 갱신은 호출자가 수행합니다.
	void KillParticles();

	// 모든 파티클들을 정리합니다.
	void KillAllParticles();
	
	// 모든 파티클들을 정리하고, Counter를 리셋합니다. 
	void Reset();

	// 모듈 갱신만 수행합니다. 수명·이동 적분과 생성 스케줄은 6단계에서 연결합니다.
	void UpdateModules(float DeltaTime);

	const FParticleModuleBinding* GetModuleBinding(const UParticleModule* Module) const;
	void* GetParticlePayload(int32 Index, const UParticleModule* Module);
	const void* GetParticlePayload(int32 Index, const UParticleModule* Module) const;
	void* GetInstancePayload(const UParticleModule* Module);
	const void* GetInstancePayload(const UParticleModule* Module) const;

	int32 GetActiveParticleCount() const { return ActiveParticles; }
	int32 GetCapacity() const { return MaxActiveParticles; }
	int32 GetParticleStride() const { return ParticleStride; }
	int32 GetParticleSize() const { return ParticleSize; }
	int32 GetInstancePayloadSize() const { return InstancePayloadSize; }
	uint32 GetParticleCounter() const { return ParticleCounter; }

	FBaseParticle* GetParticle(int32 Index);
	const FBaseParticle* GetParticle(int32 Index) const;
	int32 GetParticleSlot(int32 Index) const;

private:
	void InitializeParticleStorage(int32 InParticleStride, int32 InCapacity);
	void InitializeModuleInstances() noexcept;

	UParticleEmitter* SpriteTemplate = nullptr;
	UParticleSystemComponent* Component = nullptr;

	TUniquePtr<FParticleDataContainer> Container;
	TUniquePtr<FParticleDataContainer> InstanceContainer;
	// 배치만 복사합니다. Module 설정 객체는 Template이 계속 소유합니다.
	TArray<FParticleModuleBinding> ModuleBindings;
	int32 CurrentLODLevelIndex = 0;
	UParticleLODLevel* CurrentLODLevel = nullptr;

	// Container가 소유하는 메모리를 가리키는 비소유 포인터입니다.
	uint8* ParticleData = nullptr;
	uint16* ParticleIndices = nullptr;

	// InstanceContainer가 소유하는 비소유 포인터입니다.
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
