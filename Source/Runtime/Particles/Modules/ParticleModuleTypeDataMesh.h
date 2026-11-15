#pragma once

#include "Particles/ParticleModule.h"
#include "ParticleModuleTypeDataMesh.generated.h"

class UStaticMesh;

UCLASS()
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{
	GENERATED_BODY()

public:
	virtual bool Validate() const override;
	virtual EParticleEmitterType GetEmitterType() const override { return EParticleEmitterType::Mesh; }

	// 외부 메시 에셋은 참조만 합니다. 수명과 파일 참조 복원은 에셋 관리 계층의 책임입니다.
	UStaticMesh* GetMesh() const { return Mesh; }
	void SetMesh(UStaticMesh* InMesh) { Mesh = InMesh; }

private:
	UStaticMesh* Mesh = nullptr;
};
