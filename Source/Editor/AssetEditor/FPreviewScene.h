#pragma once

#include "Core/NameTypes.h"
#include "Math/Rotator.h"

class UEngine;
class UWorld;
class UClass;
class AActor;
class UActorComponent;

struct FPreviewSceneOptions
{
	FName WorldName = NAME_None;
	bool bCreateDefaultLighting = true;
	bool bSimulate = false;
	float SkyLightIntensity = 0.3f;
	float DirectionalLightIntensity = 1.0f;
	FRotator LightRotation = FRotator(45.0f, 30.0f, 0.0f);
};

// 엔진보다 먼저 해제해야 한다. 월드는 씬마다 독립적으로 생성한다.
class FPreviewScene
{
public:
	FPreviewScene() = default;
	~FPreviewScene();
	FPreviewScene(const FPreviewScene&) = delete;
	FPreviewScene& operator=(const FPreviewScene&) = delete;

	bool Initialize(UEngine* InEngine, const FPreviewSceneOptions& Options = {});
	void Release();
	void Tick(float DeltaTime);
	bool Step(float DeltaTime = 1.0f / 60.0f);

	AActor* AddActor(UClass* ActorClass, FName Name = NAME_None);
	UActorComponent* AddComponent(UClass* ComponentClass, FName Name = NAME_None);
	UWorld* GetWorld() const { return World; }

	void SetPaused(bool bInPaused);
	bool IsPaused() const { return bPaused; }
	void SetSimulationEnabled(bool bEnabled) { bSimulate = bEnabled; }
	bool IsSimulationEnabled() const { return bSimulate; }
	void SetTimeScale(float InTimeScale);
	float GetTimeScale() const { return TimeScale; }

private:
	bool CreateDefaultLighting(const FPreviewSceneOptions& Options);
	void AdvanceSimulation(float DeltaTime);

	UEngine* Engine = nullptr;
	UWorld* World = nullptr;
	bool bSimulate = false;
	bool bPaused = false;
	float TimeScale = 1.0f;
};
