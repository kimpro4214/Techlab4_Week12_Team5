#pragma once

#include "Core/Types.h"
#include "Editor/EditorUI/EditorPanel.h"
#include <functional>

class AActor;
class USkeletalMeshComponent;
class UBlendSpaceAnimInstance;

// 독립 에셋이 아니라 레벨 액터의 BlendSpace를 조절하는 도구다.
class FBlendSpacePanel : public IEditorPanel
{
public:
	bool Init() override { SetOpen(false); return true; }
	void Tick(float DeltaTime) override;
	void OnRender() override;
	const char* GetPanelName() const override { return "BlendSpace 1D Editor"; }
	void SetSelectedActorQuery(std::function<AActor*()> Query) { SelectedActorQuery = std::move(Query); }
	void SetIsPlayingQuery(std::function<bool()> Query) { IsPlayingQuery = std::move(Query); }
	void SetPreviewComponent(USkeletalMeshComponent* Component);
	void SetPIEComponent(USkeletalMeshComponent* Component);
	USkeletalMeshComponent* GetPreviewComponent() const;
	UBlendSpaceAnimInstance* GetPreviewInstance() const;
	UBlendSpaceAnimInstance* GetFallbackInstance() const;
	void ClearPreview();

private:
	USkeletalMeshComponent* GetLiveComponent(USkeletalMeshComponent* Component, uint32 UUID) const;
	std::function<AActor*()> SelectedActorQuery;
	std::function<bool()> IsPlayingQuery;
	USkeletalMeshComponent* PreviewComponent = nullptr;
	USkeletalMeshComponent* PIEComponent = nullptr;
	uint32 PreviewUUID = 0;
	uint32 PIEUUID = 0;
};
