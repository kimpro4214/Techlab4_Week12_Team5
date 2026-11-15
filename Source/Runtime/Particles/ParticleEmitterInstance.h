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
	
	// 같은 설정으로 재시작합니다. 입자·시간·누적값·모듈 상태를 초기화하고 생성을 허용합니다.
	void Reset();

	// 기존 월드 입자를 갱신한 뒤 이번 프레임의 신규 입자를 생성합니다.
	void Tick(
		float DeltaTime,
		const FVector& InitialLocation = FVector(0.0f),
		const FVector& InitialVelocity = FVector(0.0f),
		const FMatrix* EmitterToWorld = nullptr
	);

	// 생성만 중단합니다. Tick의 시간 진행과 기존 입자 갱신은 계속됩니다.
	void SetSpawningEnabled(bool bEnabled);
	bool IsSpawningEnabled() const { return bSpawningEnabled; }
	bool HasCompleted() const;
	double GetEmitterTime() const { return EmitterTime; }
	double GetSecondsSinceCreation() const { return SecondsSinceCreation; }
	double GetSpawnFraction() const { return SpawnFraction; }
	uint64 GetCompletedLoops() const { return CompletedLoops; }

	// 직접 호출하면 모듈만 갱신합니다. 전체 시뮬레이션은 Tick을 사용합니다.
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
	double UpdateEmitterTime(float DeltaTime);
	void UpdateParticleTimes(float DeltaTime);
	void ResetParticleParameters();
	void IntegrateParticles(float DeltaTime);
	void Spawn(double EmissionDeltaTime, float DeltaTime, const FVector& InitialLocation,
		const FVector& InitialVelocity, const FMatrix* EmitterToWorld);
	void SpawnParticlesInternal(int32 Count, double StartTime, double Increment,
		const FVector& InitialLocation, const FVector& InitialVelocity, const FMatrix* EmitterToWorld);
	void PreSpawn(FBaseParticle& Particle, const FVector& InitialLocation, const FVector& InitialVelocity);
	void PostSpawn(int32 Slot, FBaseParticle& Particle, double Age);

	UParticleEmitter* SpriteTemplate = nullptr;
	UParticleSystemComponent* Component = nullptr;

	TUniquePtr<FParticleDataContainer> Container;
	TUniquePtr<FParticleDataContainer> InstanceContainer;
	// 슬롯별 실제 경과 시간을 보관하고 RelativeTime은 매 프레임 다시 계산합니다.
	TUniquePtr<double[]> ParticleAges;
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

	// 실행 중 설정 변경은 지원하지 않으므로 초기화 시 생성 설정을 복사합니다.
	double EmitterDuration = 1.0;
	int32 EmitterLoops = 0;
	double SpawnRate = 0.0;
	double SecondsSinceCreation = 0.0;
	double EmitterTime = 0.0;
	double SpawnFraction = 0.0;
	uint64 CompletedLoops = 0;
	bool bEmitterEnabled = false;
	bool bSpawningEnabled = true;
	bool bEmitterFinished = true;
};
