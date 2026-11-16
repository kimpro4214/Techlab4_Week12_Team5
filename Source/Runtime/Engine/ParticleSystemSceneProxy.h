#pragma once

#include "Components/BillboardComponent.h"
#include "Containers/Array.h"
#include "Engine/PrimitiveSceneProxy.h"

class UParticleSystemComponent;

class FParticleSystemSceneProxy : public FPrimitiveSceneProxy
{
public:
	explicit FParticleSystemSceneProxy(UParticleSystemComponent* InComponent);

	void UpdateDynamicData() override;
	void GatherMeshElements(FRenderQueue& Queue, const FViewContext& View) const override;

private:
	struct FParticleRenderData
	{
		FVector Location;
		float Scale;
		FSubUVConstants Params;
	};

	UParticleSystemComponent* ParticleComponent;
	UStaticMesh* QuadMesh = nullptr;
	UMaterial* Material = nullptr;
	TArray<FParticleRenderData> RenderParticles;
};
