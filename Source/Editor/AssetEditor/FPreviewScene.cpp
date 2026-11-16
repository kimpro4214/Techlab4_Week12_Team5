#include "EnginePCH.h"
#include "Editor/AssetEditor/FPreviewScene.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include <cmath>

FPreviewScene::~FPreviewScene()
{
	Release();
}

bool FPreviewScene::Initialize(UEngine* InEngine, const FPreviewSceneOptions& Options)
{
	if (World) return Engine == InEngine;
	if (!InEngine) return false;

	Engine = InEngine;
	World = UWorld::CreateWorld(EWorldType::EditorPreview, false, Options.WorldName);
	if (!World)
	{
		Engine = nullptr;
		return false;
	}
	Engine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
	bSimulate = Options.bSimulate;
	bPaused = false;
	TimeScale = 1.0f;

	if (Options.bCreateDefaultLighting && !CreateDefaultLighting(Options))
	{
		Release();
		return false;
	}
	World->GetScene().UpdateAllTransforms();
	return true;
}

void FPreviewScene::Release()
{
	if (!World) return;
	UWorld* ReleasedWorld = World;
	World = nullptr;

	// EditorPreview 컨텍스트는 월드와 액터까지 함께 파괴한다.
	if (Engine->GetWorldContextFromWorld(ReleasedWorld)) Engine->DestroyWorldContext(ReleasedWorld);
	else Engine->DestroyWorld(ReleasedWorld);
	Engine = nullptr;
	bSimulate = false;
	bPaused = false;
	TimeScale = 1.0f;
}

AActor* FPreviewScene::AddActor(UClass* ActorClass, FName Name)
{
	if (!World) return nullptr;
	AActor* Actor = World->SpawnActor(ActorClass, Name);
	if (Actor) Actor->SetActorEnableCollision(false);
	return Actor;
}

UActorComponent* FPreviewScene::AddComponent(UClass* ComponentClass, FName Name)
{
	if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass())) return nullptr;
	AActor* Actor = AddActor(AActor::StaticClass());
	if (!Actor) return nullptr;

	UActorComponent* Component = Actor->AddComponent(ComponentClass, Name);
	if (!Component) World->DestroyActor(Actor);
	return Component;
}

bool FPreviewScene::CreateDefaultLighting(const FPreviewSceneOptions& Options)
{
	// 일반 액터에 라이트 컴포넌트만 추가해 편집용 아이콘이 나타나지 않게 한다.
	AActor* Actor = AddActor(AActor::StaticClass(), "PreviewLights");
	if (!Actor) return false;
	auto* Sky = Cast<USkyLightComponent>(Actor->AddComponent(USkyLightComponent::StaticClass(), "PreviewAmbient"));
	auto* Sun = Cast<UDirectionalLightComponent>(Actor->AddComponent(UDirectionalLightComponent::StaticClass(), "PreviewDirectional"));
	if (!Sky || !Sun) return false;

	Sky->SetIntensity(Options.SkyLightIntensity);
	Sun->SetRelativeRotation(Options.LightRotation);
	Sun->SetIntensity(Options.DirectionalLightIntensity);
	return true;
}

void FPreviewScene::SetPaused(bool bInPaused)
{
	bPaused = bInPaused;
	if (World) World->SetPaused(bPaused);
}

void FPreviewScene::SetTimeScale(float InTimeScale)
{
	if (std::isfinite(InTimeScale) && InTimeScale >= 0.0f) TimeScale = InTimeScale;
}

void FPreviewScene::AdvanceSimulation(float DeltaTime)
{
	World->Tick(DeltaTime);
	// 시뮬레이션 중 변경된 변환을 같은 프레임의 렌더링에 반영한다.
	World->GetScene().UpdateAllTransforms();
}

void FPreviewScene::Tick(float DeltaTime)
{
	if (!World) return;
	const float ScaledDelta = DeltaTime * TimeScale;
	if (bSimulate && !bPaused && std::isfinite(ScaledDelta) && ScaledDelta > 0.0f)
	{
		AdvanceSimulation(ScaledDelta);
	}
	else
	{
		// 수동 포즈 평가·일시정지 중에도 편집한 위치와 조명은 갱신한다.
		World->GetScene().UpdateAllTransforms();
	}
}

bool FPreviewScene::Step(float DeltaTime)
{
	if (!World || !bSimulate || !bPaused || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) return false;
	// 한 프레임 진행은 배속과 무관하게 요청한 시간만 진행한다.
	World->SetPaused(false);
	AdvanceSimulation(DeltaTime);
	World->SetPaused(true);
	return true;
}
