#pragma once

#include "Editor/AssetEditor/AssetEditor.h"
#include "Editor/AssetEditor/FPreviewScene.h"
#include "Editor/Viewports/EditorViewportClient.h"
#include "Camera/ViewportCameraMovement.h"
#include <functional>

class UEngine;
class UParticleSystem;
class UParticleSystemComponent;
class UParticleEmitter;
class UParticleModule;
struct FEditorSettings;

class FParticleEditor : public FAssetEditor
{
public:
	using FPreviewRenderer = std::function<void(FParticleEditor&)>;

	FParticleEditor(UEngine* InEngine, FEditorManager& InManager, const FAssetEditorKey& InKey,
		UParticleSystem* InSystem, const FEditorSettings* InSettings, FPreviewRenderer InRenderer);
	~FParticleEditor() override;

	////////////////////////////////////////////////////////////
	// 에디터 공통 인터페이스
	////////////////////////////////////////////////////////////

	void Tick(float DeltaTime) override;
	void RenderPreview() override;
	void OnRender(bool) override {}
	bool IsFocused() const override { return bFocused; }
	bool IsDirty() const override;
	bool CanSave() const override { return System && !IsClosing(); }
	bool Save() override;

	////////////////////////////////////////////////////////////
	// 패널 상태 및 뷰포트 제어
	////////////////////////////////////////////////////////////

	// 매 UI 프레임에 표시·포커스·입력 상태를 초기화하고 각 패널의 상태를 전달받습니다.
	void BeginUIFrame();
	void ReportPanelFocus(bool bInFocused) { bFocused |= bInFocused; }
	void SetViewportState(uint32 Width, uint32 Height, const FViewportCameraInput& Input);
	void FocusSystem();

	////////////////////////////////////////////////////////////
	// 프리뷰 시뮬레이션 제어
	////////////////////////////////////////////////////////////

	void SetPaused(bool bPaused) { PreviewScene.SetPaused(bPaused); }
	void Restart();
	void Step() { if (bPreviewValid) PreviewScene.Step(); }
	void SetTimeScale(float Value) { PreviewScene.SetTimeScale(Value); }

	////////////////////////////////////////////////////////////
	// 이미터·모듈 선택 및 변경 반영
	////////////////////////////////////////////////////////////

	void SelectEmitter(int32 Index);
	void SelectModule(int32 EmitterIndex, UParticleModule* Module);
	UParticleEmitter* GetSelectedEmitter() const;
	UParticleModule* GetSelectedModule() const { return SelectedModule; }
	int32 GetSelectedEmitterIndex() const { return SelectedEmitterIndex; }
	bool RefreshSystem();

	////////////////////////////////////////////////////////////
	// Getter / Setter
	////////////////////////////////////////////////////////////

	bool IsPaused() const { return PreviewScene.IsPaused(); }
	float GetTimeScale() const { return PreviewScene.GetTimeScale(); }
	int32 GetActiveParticleCount() const;
	const FString& GetLastError() const { return LastError; }

	bool IsViewportVisible() const { return bViewportVisible; }
	bool IsRenderGrid() const { return bRenderGrid; }
	void SetRenderGrid(bool bValue) { bRenderGrid = bValue; }

	UParticleSystem* GetSystem() const { return System; }
	UParticleSystemComponent* GetParticleComponent() const { return ParticleComponent; }
	UWorld* GetPreviewWorld() const { return PreviewScene.GetWorld(); }
	FEditorViewportClient* GetViewportClient() { return &ViewportClient; }

protected:
	////////////////////////////////////////////////////////////
	// 탭 생성 및 리소스 수명 관리
	////////////////////////////////////////////////////////////

	TUniquePtr<IEditorTab> CreateTab() override;
	bool InitializeResources() override;
	void ShutdownResources() override;
	bool DiscardChanges() override;

private:
	// 외부 참조 및 렌더링 콜백
	UEngine* Engine = nullptr;
	UParticleSystem* SourceSystem = nullptr;
	UParticleSystemComponent* ParticleComponent = nullptr;
	const FEditorSettings* Settings = nullptr;
	FPreviewRenderer PreviewRenderer;

	// 에디터가 소유하는 편집 복사본과 마지막 저장 기준
	UParticleSystem* System = nullptr;
	TArray<uint8> SavedSnapshot;
	FString LastError;

	// 선택은 편집 복사본 안의 LOD 0에만 적용합니다.
	int32 SelectedEmitterIndex = -1;
	UParticleModule* SelectedModule = nullptr;
	bool bPreviewValid = true;

	// 에디터 전용 프리뷰 리소스
	FPreviewScene PreviewScene;
	FEditorViewportClient ViewportClient;
	FViewportCameraInput CameraInput;

	// 뷰포트 크기 및 표시·입력 상태
	uint32 ViewportWidth = 0;
	uint32 ViewportHeight = 0;
	bool bViewportVisible = false;
	bool bFocused = false;
	bool bRenderGrid = true;
	bool bInitialFocusPending = true;
};
