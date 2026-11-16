#include "EnginePCH.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleSpawn.h"
#include "Render/Particle/ParticleModule.h"
#include "Components/ParticleSystemComponent.h"
#include "Core/HAL/UnrealMemory.h"
#include <algorithm>

FParticleEmitterInstance::~FParticleEmitterInstance()
{
	FreeParticleData();
	FMemory::Free(InstanceData);
}

bool FParticleEmitterInstance::Init(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent)
{
	if (!InTemplate || !InComponent)
		return false;

	SpriteTemplate = InTemplate;
	Component = InComponent;
	CurrentLODLevelIndex = 0;
	CurrentLODLevel = SpriteTemplate->GetLODLevel(CurrentLODLevelIndex);
	if (!CurrentLODLevel)
		return false;

	// Required·Spawn이 없으면 블록 크기도 생성량도 알 수 없으므로, 할당 전에 실패시킨다
	UParticleModuleRequired* Required = CurrentLODLevel->GetRequiredModule();
	if (!Required || !CurrentLODLevel->GetSpawnModule())
		return false;

	// 칸 크기: 지금은 FBaseParticle뿐. 모듈별 Payload가 생기면 여기에 더해진다.
	ParticleSize = SpriteTemplate->GetParticleSize();
	if (ParticleSize == 0) return false;
	// 칸 간격을 16의 배수로 올린다 → 모든 칸의 시작 주소가 16바이트 정렬 (Address Align)
	ParticleStride = static_cast<int32>(FMemory::QuantizeSize(ParticleSize, 16));

	ActiveParticles = 0;
	ParticleCounter = 0;
	SpawnFraction = 0.0f;
	EmitterTime = 0.0f;

	const int32 MaxParticles = std::clamp(Required->GetMaxParticles(), 1, 65535);
	
	FMemory::Free(InstanceData);
	InstanceData = nullptr;

	if (!AllocateParticleData(MaxParticles))
		return false;

	const uint64 InstanceBytes = SpriteTemplate->GetRequiredInstanceBytes();
	if (InstanceBytes == 0)
		return true;

	InstanceData = static_cast<uint8*>(FMemory::Malloc(InstanceBytes));
	if (!InstanceData)
	{
		FreeParticleData();
		return false;
	}

	FMemory::Memzero(InstanceData, InstanceBytes);

	for (UParticleModule* Module : CurrentLODLevel->GetModules())
	{
		if (uint8* Data = GetModuleInstanceData(Module))
			Module->PrepPerInstanceBlock(*this, Data);
	}
	return true;
}

void FParticleEmitterInstance::Spawn(float DeltaTime)
{
	UParticleModuleSpawn* SpawnModule = CurrentLODLevel->GetSpawnModule();
	if (!SpawnModule)
		return;

	// 초당 Rate개 → 이번 프레임 몫 = Rate × dt
	// 예) Rate 30, dt 0.016 → 0.48개: 이번엔 0개, 다음 프레임 0.96, 그다음 1.44 → 1개 생성
	const float SpawnRate = SpawnModule->GetRate() * SpawnModule->GetRateScale();
	const float OldFraction = SpawnFraction;
	const float NewParticles = OldFraction + SpawnRate * DeltaTime;
	const int32 Count = static_cast<int32>(NewParticles);
	SpawnFraction = NewParticles - static_cast<float>(Count);

	if (Count > 0 && SpawnRate > 0.0f)
	{
		const float SpawnInterval = 1.0f / SpawnRate;
		const float FirstSpawnTime = DeltaTime - (1.0f - OldFraction) * SpawnInterval;
		SpawnParticles(Count, EmitterToSimulation.GetOrigin(), FirstSpawnTime, SpawnInterval);
	}
}

void FParticleEmitterInstance::SpawnParticles(int32 Count, const FVector& InitialLocation,
	float FirstSpawnTime, float SpawnInterval)
{
	for (int32 i = 0; i < Count; ++i)
	{
		if (ActiveParticles >= MaxActiveParticles)
			break;   // 블록이 가득 참

		// 살아있는 인덱스 바로 뒤가 다음 빈 칸이다
		const int32 CurrentIndex = ParticleDataBlock.ParticleIndices[ActiveParticles];
		DECLARE_PARTICLE_PTR(Particle, ParticleDataBlock.ParticleData + ParticleStride * CurrentIndex);

		PreSpawn(Particle, InitialLocation);

		const float SpawnTime = std::max(0.0f, FirstSpawnTime - i * SpawnInterval);
		for (UParticleModule* Module : CurrentLODLevel->GetSpawnModules())
		{
			const int32 Offset = SpriteTemplate->GetModuleOffset(Module);
			Module->Spawn({ *this, Offset, SpawnTime, Particle });
		}

		Particle->Location += Particle->Velocity * SpawnTime;
		Particle->Rotation += Particle->RotationRate * SpawnTime;
		Particle->RelativeTime += Particle->OneOverMaxLifetime * SpawnTime;

		++ActiveParticles;
		++ParticleCounter;
	}
}

void FParticleEmitterInstance::UpdateTransforms()
{
	UParticleModuleRequired* Required = CurrentLODLevel->GetRequiredModule();
	const FMatrix ComponentToWorld = Component->GetWorldMatrix();

	// Emitter 로컬 오프셋 (Required의 EmitterOrigin)
	FMatrix EmitterToComponent = FMatrix::Identity;
	if (Required)
	{
		const FVector Origin = Required->GetEmitterOrigin();
		EmitterToComponent.M[3][0] = Origin.X;
		EmitterToComponent.M[3][1] = Origin.Y;
		EmitterToComponent.M[3][2] = Origin.Z;
	}

	if (Required && Required->IsUseLocalSpace())
	{
		// 파티클을 컴포넌트 로컬에 저장 → 그릴 때 컴포넌트 행렬로 월드 변환
		EmitterToSimulation = EmitterToComponent;
		SimulationToWorld = ComponentToWorld;
	}
	else
	{
		// 파티클을 월드에 저장 → 그대로 그림
		EmitterToSimulation = EmitterToComponent * ComponentToWorld;   
		SimulationToWorld = FMatrix::Identity;
	}
}

void FParticleEmitterInstance::Tick(float DeltaTime)
{
	if (!CurrentLODLevel || !Component)
		return;

	EmitterTime += DeltaTime;

	// 컴포넌트가 움직였을 수 있으므로 생성 위치/공간 변환을 먼저 갱신
	UpdateTransforms();

	KillParticles();                     // 1. 지난 프레임에 수명이 다한 파티클 제거
	ResetParticleParameters(DeltaTime);  // 2. Base값 복원 + 나이 증가
	Tick_ModuleUpdate(DeltaTime);        // 3. Update 모듈이 Base값을 가공
	Integrate(DeltaTime);                // 4. 기존 파티클의 위치·회전 갱신
	Spawn(DeltaTime);                    // 5. 새 파티클은 SpawnTime만큼만 진행
}

uint8* FParticleEmitterInstance::GetModuleInstanceData(const UParticleModule* Module)
{
	if (!InstanceData)
		return nullptr;

	const uint64* Offset = SpriteTemplate->FindModuleInstanceOffset(Module);
	return Offset ? InstanceData + *Offset : nullptr;
}

bool FParticleEmitterInstance::FillReplayData(FDynamicEmitterReplayDataBase& OutData)
{
	if (ActiveParticles <= 0 || !ParticleDataBlock.ParticleData)
		return false;

	OutData.ActiveParticleCount = ActiveParticles;
	OutData.ParticleStride      = ParticleStride;
	OutData.ParticleData        = ParticleDataBlock.ParticleData;
	OutData.ParticleIndices     = ParticleDataBlock.ParticleIndices;
	OutData.SimulationToWorld   = SimulationToWorld;
	// 월드 공간 시뮬레이션이면 SimulationToWorld가 단위 행렬이라, 이미터 위치를 따로 계산해서 넘긴다
	OutData.EmitterWorldOrigin  = SimulationToWorld.TransformPosition(EmitterToSimulation.GetOrigin());
	return true;
}

void FParticleEmitterInstance::Tick_ModuleUpdate(float DeltaTime)
{
	for (UParticleModule* Module : CurrentLODLevel->GetUpdateModules())
	{
		const int32 Offset = SpriteTemplate->GetModuleOffset(Module);
		Module->Update({ *this, Offset, DeltaTime });
	}
}

void FParticleEmitterInstance::Integrate(float DeltaTime)
{
	FParticleEmitterInstance* Owner = this;
	BEGIN_UPDATE_LOOP
		Particle.Location += Particle.Velocity * DeltaTime;
	Particle.Rotation += Particle.RotationRate * DeltaTime;
	END_UPDATE_LOOP
}


void FParticleEmitterInstance::ResetParticleParameters(float DeltaTime)
{
	FParticleEmitterInstance* Owner = this;
	BEGIN_UPDATE_LOOP
		// Update 모듈은 "Base에서 얼마나 바꿀지"만 계산한다 → 매 프레임 되돌려야 값이 누적되지 않는다
		Particle.Velocity = Particle.BaseVelocity;
	Particle.Size = Particle.BaseSize;
	Particle.RotationRate = Particle.BaseRotationRate;
	Particle.Color = Particle.BaseColor;
	Particle.RelativeTime += Particle.OneOverMaxLifetime * DeltaTime;
	END_UPDATE_LOOP
}

void FParticleEmitterInstance::KillParticles()
{
	FParticleEmitterInstance* Owner = this;
	BEGIN_UPDATE_LOOP
		if (Particle.RelativeTime >= 1.0f)
			KillParticle(i);
	END_UPDATE_LOOP
}

void FParticleEmitterInstance::KillParticle(int32 Index)
{
	if (Index < 0 || Index >= ActiveParticles)
		return;

	// 죽는 인덱스를 살아있는 구간의 맨 끝과 바꾸고 구간을 하나 줄인다 → O(1), 메모리 이동 없음
	// 다음 Spawn이 칸을 재사용할 때 PreSpawn이 Memzero한다.
	uint16* Indices = ParticleDataBlock.ParticleIndices;
	std::swap(Indices[Index], Indices[ActiveParticles - 1]);
	--ActiveParticles;
}

void FParticleEmitterInstance::PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation)
{
	// Kill은 칸을 지우지 않으므로, 재사용되는 칸에는 이전 파티클 값이 남아 있다 → 칸 전체를 0으로
	FMemory::Memzero(Particle, ParticleStride);

	Particle->Location = InitialLocation;
	Particle->OneOverMaxLifetime = 1.0f;    // 기본 수명 1초 (Lifetime 모듈이 덮어쓴다)
	Particle->Size = Particle->BaseSize = FVector(1.0f, 1.0f, 1.0f);
	Particle->Color = Particle->BaseColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
}


bool FParticleEmitterInstance::AllocateParticleData(int32 MaxParticles)
{
	FreeParticleData();
	MaxActiveParticles = MaxParticles;

	// [ 파티클 데이터 (Stride × Max) | 인덱스 (uint16 × Max) ] 를 한 번에 할당 
	// 실행 중에는 더 할당하지 않는다.
	ParticleDataBlock.ParticleDataNumBytes = ParticleStride * MaxParticles;
	ParticleDataBlock.ParticleIndicesNumShorts = MaxParticles;
	ParticleDataBlock.MemBlockSize =
		ParticleDataBlock.ParticleDataNumBytes + ParticleDataBlock.ParticleIndicesNumShorts * static_cast<int32>(sizeof(uint16));

	ParticleDataBlock.ParticleData = static_cast<uint8*>(FMemory::Malloc(ParticleDataBlock.MemBlockSize));
	if (!ParticleDataBlock.ParticleData)
		return false;

	// 인덱스 영역은 데이터 영역 바로 뒤
	ParticleDataBlock.ParticleIndices =
		reinterpret_cast<uint16*>(ParticleDataBlock.ParticleData + ParticleDataBlock.ParticleDataNumBytes);

	// 처음엔 칸 i에 인덱스 i. Spawn/Kill이 이 순서를 섞는다
	for (int32 i = 0; i < MaxParticles; ++i)
		ParticleDataBlock.ParticleIndices[i] = static_cast<uint16>(i);

	return true;
}

void FParticleEmitterInstance::FreeParticleData()
{
	// ParticleIndices는 같은 블록 안이므로 ParticleData 하나만 해제한다
	if (ParticleDataBlock.ParticleData)
		FMemory::Free(ParticleDataBlock.ParticleData);

	ParticleDataBlock = FParticleDataContainer{};
	ActiveParticles = 0;
	MaxActiveParticles = 0;
}
