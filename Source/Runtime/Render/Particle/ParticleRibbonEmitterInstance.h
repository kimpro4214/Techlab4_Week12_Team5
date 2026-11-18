#pragma once

#include "Render/Particle/ParticleEmitterInstance.h"   // 렌더 헤더는 include하지 않는다

struct FDynamicRibbonEmitterData;
class UParticleModuleTypeDataRibbon;

// 리본(궤적) 이미터의 실행 상태
// 파티클 하나 = 띠 위의 점 하나. 시뮬레이션(Spawn/Update/Kill)은 부모를 그대로 쓰고,
// 살아있는 파티클을 생성 순서대로 이어 렌더 쪽에 점 목록으로 넘기는 부분만 다르다
struct FParticleRibbonEmitterInstance : public FParticleEmitterInstance
{
	explicit FParticleRibbonEmitterInstance(UParticleModuleTypeDataRibbon* InRibbonTypeData);
	~FParticleRibbonEmitterInstance() override;   // TUniquePtr<불완전 타입>이라 정의는 .cpp에 둔다

	bool Init(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent) override;

	// Tick은 오버라이드하지 않는다. 생성 순서는 SpawnParticles가 각 파티클에 넣는 ParticleId(= ParticleCounter++)로 알 수 있다

	FDynamicEmitterDataBase* GetDynamicData() override;

protected:
	bool FillReplayData(FDynamicEmitterReplayDataBase& OutData) override;

private:
	UParticleModuleTypeDataRibbon* RibbonTypeData = nullptr;

	TArray<int32> SortedSlots;   // 살아있는 파티클의 칸 번호를 생성 순서(최신 → 오래된)로 정렬한 임시 배열

	// 마디가 태어날 때의 누적 거리 (텍스처 U가 꼬리로 흐르지 않게)
	// 칸은 재사용되므로, 칸에 마지막으로 기록한 ParticleId와 다르면 새로 태어난 마디로 본다
	TArray<uint32> SlotParticleIds;
	TArray<float>  SlotBirthDistances;
	bool    bHasHead = false;              // 아직 한 번도 마디를 기록하지 않았으면 false
	FVector LastHeadPosition = FVector::ZeroVector;   // 가장 최근에 태어난 마디의 월드 위치
	float   LastHeadDistance = 0.0f;       // 그 마디의 누적 거리

	TUniquePtr<FDynamicRibbonEmitterData> DynamicData;
};
