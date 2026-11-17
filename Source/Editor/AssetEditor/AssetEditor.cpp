#include "EnginePCH.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include "Editor/AssetEditor/AssetEditorTab.h"
#include "Editor/AssetEditor/EditorManager.h"

bool FAssetEditorKey::operator==(const FAssetEditorKey& Other) const
{
	return EditorType == Other.EditorType && AssetPath == Other.AssetPath && SubAssetName == Other.SubAssetName;
}

FString FAssetEditorKey::GetTabId() const
{
	// 이름에 구분 문자가 포함되어도 충돌하지 않도록 각 요소의 길이를 포함한다.
	return std::format("{}:{}{}:{}{}:{}", EditorType.size(), EditorType,
		AssetPath.size(), AssetPath, SubAssetName.size(), SubAssetName);
}

FAssetEditor::FAssetEditor(FEditorManager& InManager, const FAssetEditorKey& InKey)
	: Manager(InManager), Key(InKey)
{
}

FAssetEditor::~FAssetEditor()
{
	// 파생 자원 정리는 매니저가 소멸 전에 Shutdown으로 수행한다.
	if (Tab) Manager.RemoveEditorTab(Tab.get());
}

bool FAssetEditor::Initialize()
{
	if (bShutdown) return false;
	if (bInitialized) return true;

	if (!InitializeResources())
	{
		Shutdown();
		return false;
	}

	Tab = CreateTab();
	if (!Tab)
	{
		Shutdown();
		return false;
	}

	Manager.AddEditorTab(Tab.get());
	bInitialized = true;
	Activate();
	return true;
}

void FAssetEditor::Shutdown()
{
	if (bShutdown) return;
	bShutdown = true;
	CloseState = EAssetEditorCloseState::Confirmed;

	if (Tab)
	{
		Manager.RemoveEditorTab(Tab.get());
		Tab.reset();
	}
	ShutdownResources();
	bInitialized = false;
}

TUniquePtr<IEditorTab> FAssetEditor::CreateTab()
{
	return MakeUnique<FAssetEditorTab>(*this);
}

void FAssetEditor::Activate()
{
	if (Tab && !IsClosing()) Manager.ActivateEditorTab(Tab.get());
}

void FAssetEditor::Close()
{
	if (bShutdown || CloseState != EAssetEditorCloseState::None) return;
	CloseState = IsDirty() ? EAssetEditorCloseState::AwaitingDecision : EAssetEditorCloseState::Confirmed;
	if (CloseState == EAssetEditorCloseState::AwaitingDecision) Activate();
}

bool FAssetEditor::SaveAndClose()
{
	if (CloseState != EAssetEditorCloseState::AwaitingDecision || !CanSave() || !Save() || IsDirty()) return false;
	CloseState = EAssetEditorCloseState::Confirmed;
	return true;
}

bool FAssetEditor::DiscardAndClose()
{
	if (CloseState != EAssetEditorCloseState::AwaitingDecision || !DiscardChanges()) return false;
	CloseState = EAssetEditorCloseState::Confirmed;
	return true;
}

void FAssetEditor::CancelClose()
{
	if (CloseState == EAssetEditorCloseState::AwaitingDecision) CloseState = EAssetEditorCloseState::None;
}
