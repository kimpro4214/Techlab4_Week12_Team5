#pragma once

#include "SceneComponent.h"
#include "Math/Color.h"
#include "Render/Material.h"
#include "SkySphereComponent.generated.h"

UCLASS()
class USkySphereComponent final : public USceneComponent
{
	GENERATED_BODY()

public:
	UMaterial* GetBackgroundMaterial() const { return BackgroundMaterial; }
	void SetBackgroundMaterial(UMaterial* Material) { BackgroundMaterial = Material; }
	FColor GetBackgroundColor() const { return BackgroundColor; }
	void SetBackgroundColor(FColor Color) { BackgroundColor = Color; }

protected:
	void OnRegister() override;
	void OnUnregister() override;

private:
	UPROPERTY(EditAnywhere)
	UMaterial* BackgroundMaterial = nullptr;

	UPROPERTY(EditAnywhere)
	FColor BackgroundColor = FColor(0, 0, 0, 255);
};
