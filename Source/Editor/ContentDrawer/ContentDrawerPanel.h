#pragma once

#include "Editor/EditorUI/EditorPanel.h"
#include "Core/Delegate/Delegate.h"

#include <filesystem>

namespace fs = std::filesystem;

class UAnimSequence;
class USkeletalMesh;

class FContentDrawerPanel : public IEditorPanel
{
public:
	bool Init() override;
	void Tick(float DeltaTime) override;
	void OnRender() override;
	const char* GetPanelName() const override { return "Content Drawer"; }
	void RenderFolderTree(const fs::path& Path, fs::path& SelectedPath);

	////////////////////////////////////////////////////////////
	// 에셋 열기 및 생성 요청
	////////////////////////////////////////////////////////////

	TDelegate<const FString&> OnOpenWidget;
	TDelegate<const FString&> OnCreateWidget;
	TDelegate<const FString&> OnOpenStatic;
	TDelegate<const FString&> OnOpenParticle;
	TDelegate<const FString&, FString&> OnCreateParticle;
	TDelegate<const FString&> OnOpenSkeletal;
	TDelegate<UAnimSequence*, USkeletalMesh*> OnOpenAnimation;

private:
	void DrawAnimationAssets(const fs::path& FbxPath, float ThumbnailSize);
	void DrawCreateParticlePopup();

	char NewAssetName[64] = "NewWidget";
	bool bOpenCreateWidget = false;
	FString ParticleCreateError;

	fs::path AssetRootPath = "Assets";
	fs::path CurrentPath = "Assets";

	const FString FolderIconPath = "Assets/Icons/Folder.png";
	const FString FileIconPath = "Assets/Icons/File.png";
	
};
