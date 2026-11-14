#include "EnginePCH.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "ParticleModuleSpawn.h"
#include "ParticleModuleRequired.h"
#include "Components/ParticleSystemComponent.h"



FParticleEmitterInstance::~FParticleEmitterInstance()
{
	// ParticleData가 블록 전체의 시작 주소. ParticleIndices는 같은 블록 안의 위치라 따로 해제하지 않음
	if (ParticleDataBlock.ParticleData != nullptr)
	{
		FMemory::Free(ParticleDataBlock.ParticleData);
	}
	ParticleDataBlock.ParticleData = nullptr;
	ParticleDataBlock.ParticleIndices = nullptr;
}

bool FParticleEmitterInstance::Init(UParticleEmitter* Emitter, UParticleSystemComponent* ParticleComponent)
{
	if (Emitter != nullptr)		SpriteTemplate = Emitter;	else return false;
	if (ParticleComponent != nullptr)		Component = ParticleComponent;	else return false;

	if (SpriteTemplate->GetLODLevels().Num() == 0) return false;
	else CurrentLODLevel = SpriteTemplate->GetLODLevels()[0];

	// Required·Spawn 모듈은 Emitter마다 필수. 없으면 설정·생성량을 알 수 없으므로 메모리 할당 전에 실패 처리
	if (CurrentLODLevel->GetRequiredModule() == nullptr) return false;
	if (CurrentLODLevel->GetSpawnModule() == nullptr) return false;

	ParticleSize = sizeof(FBaseParticle);
	ParticleStride = FMemory::QuantizeSize(ParticleSize, 16);		// ParticleSize 바탕으로 16배수로 맞춤

	ActiveParticles = 0;
	MaxActiveParticles = 100;
	SpawnFraction = 0;

	ParticleDataBlock.ParticleDataNumBytes = ParticleStride * MaxActiveParticles;
	ParticleDataBlock.ParticleIndicesNumShorts = MaxActiveParticles;
	ParticleDataBlock.MemBlockSize = ParticleDataBlock.ParticleDataNumBytes + ParticleDataBlock.ParticleIndicesNumShorts * sizeof(uint16);	// Block 전체 Size = Particle 영역 + Index 영역

	ParticleDataBlock.ParticleData = (uint8*)FMemory::Malloc(ParticleDataBlock.MemBlockSize);	// 블럭 전체 메모리 할당
	if (ParticleDataBlock.ParticleData == nullptr) return false;
	ParticleDataBlock.ParticleIndices = (uint16*)(ParticleDataBlock.ParticleData + ParticleDataBlock.ParticleDataNumBytes);	// Indices 영역 시작지점
	if (ParticleDataBlock.ParticleIndices == nullptr) return false;

	for (int i = 0;i < MaxActiveParticles;i++)
	{
		ParticleDataBlock.ParticleIndices[i] = i;	// Indices 배열에 Indices 채움
	}

	return true;
}

void FParticleEmitterInstance::Spawn(float DeltaTime)
{
	float EmitterRate = CurrentLODLevel->GetSpawnModule()->GetRate();
	float EmitterRateScale = CurrentLODLevel->GetSpawnModule()->GetRateScale();
	float ParticleNum = EmitterRate * EmitterRateScale;	
	float CurrentFrameSpawn = SpawnFraction + DeltaTime * ParticleNum;		// 이번 프레임 스폰량 계산
	int32 CurrentCount = int32(CurrentFrameSpawn);	// 정수 부분 만큼만 파티클 개수로 렌더링
	SpawnFraction = CurrentFrameSpawn - CurrentCount;

	if (CurrentCount > 0) SpawnParticles(CurrentCount);
}

void  FParticleEmitterInstance::SpawnParticles(int32 Count)
{
	for (int i = 0;i < Count;i++)
	{
		if (ActiveParticles >= MaxActiveParticles) break;

		int32 CurrentIndex = ParticleDataBlock.ParticleIndices[ActiveParticles];
		DECLARE_PARTICLE_PTR(Particle, (ParticleDataBlock.ParticleData + ParticleStride * CurrentIndex));

		FMemory::Memzero(Particle, ParticleStride);	// 기존 Particle 값 초기화
		FVector SpawnOrigin;
		if(CurrentLODLevel->GetRequiredModule()->IsUseLocalSpace()) SpawnOrigin = CurrentLODLevel->GetRequiredModule()->GetEmitterOrigin();
		else SpawnOrigin = Component->GetWorldMatrix().TransformPosition(CurrentLODLevel->GetRequiredModule()->GetEmitterOrigin());	// Component 기준 로컬 좌표를 월드 좌표로 변환

		for (UParticleModule* Module : CurrentLODLevel->GetModules()) Module->Spawn(this, 0, 0.0f, Particle);	// 전체 Emitter Module에 대해서 Particle Update하도록 Spawn 진행

		Particle->Location = SpawnOrigin;
		ActiveParticles++;
	}
}

