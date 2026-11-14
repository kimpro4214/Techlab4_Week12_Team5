#pragma once

#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleRequired.generated.h"

class UMaterial;

// Emitter에 꼭 필요한 설정 
// 어떤 Material을 쓸지, Payload 블록 크기 등 Emitter의 기본 설정 
// Modules 목록에 넣지 않고 LODLevel이 따로 들고 있다. Spawn/Update 호출 대상이 아니다.
UCLASS()
class UParticleModuleRequired : public UParticleModule
{
	GENERATED_BODY()
public:
	UMaterial* GetMaterial() const { return Material; }
	int32 GetMaxParticles() const { return MaxParticles; }
	const FVector& GetEmitterOrigin() const { return EmitterOrigin; }
	bool IsUseLocalSpace() const { return bUseLocalSpace; }

	void SetMaterial(UMaterial* InMaterial) { Material = InMaterial; }
	void SetMaxParticles(int32 InMax) { MaxParticles = InMax; }
	void SetEmitterOrigin(const FVector& InOrigin) { EmitterOrigin = InOrigin; }
	void SetUseLocalSpace(bool bInLocal) { bUseLocalSpace = bInLocal; }

private:
	UMaterial* Material = nullptr;              // 렌더 단계에서 사용

	// Payload 블록 크기가 이 값으로 정해진다. 인덱스가 uint16이라 65535를 넘을 수 없다.
	int32 MaxParticles = 500;
	
	// 컴포넌트 기준 로컬 오프셋
	FVector EmitterOrigin = FVector::ZeroVector; 	
	
	// true : 파티클이 컴포넌트 로컬 좌표로 저장된다 → 액터를 움직이면 파티클도 같이 움직인다
	// false: 월드 좌표로 저장된다 → 이미 나온 파티클은 그 자리에 남는다
	bool bUseLocalSpace = false;
};