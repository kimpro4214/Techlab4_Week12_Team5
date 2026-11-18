#include "EnginePCH.h"
#include "Editor/AssetEditor/EditorManager.h"

#include <cctype>

FEditorManager::~FEditorManager()
{
	Shutdown();
}

bool FEditorManager::Init()
{
	if (bShuttingDown) return false;
	if (bInitialized) return true;

	LevelEditorTab = MakeUnique<FLevelEditorTab>();
	if (!LevelEditorTab->Init())
	{
		LevelEditorTab.reset();
		return false;
	}
	TabContainer.AddTab(LevelEditorTab.get());
	TabContainer.ActivateTab(LevelEditorTab.get());
	bInitialized = true;
	return true;
}

bool FEditorManager::NormalizeKey(const FAssetEditorKey& InKey, FAssetEditorKey& OutKey)
{
	if (InKey.EditorType.empty() || InKey.AssetPath.empty()) return false;
	std::error_code Error;
	const std::filesystem::path AbsolutePath = std::filesystem::absolute(InKey.AssetPath, Error);
	if (Error) return false;
	const std::filesystem::path Path = std::filesystem::weakly_canonical(AbsolutePath, Error);
	if (Error || !Path.is_absolute()) return false;

	OutKey = InKey;
	OutKey.AssetPath = Path.generic_string();
	// Windows 경로의 대소문자 차이로 같은 파일의 편집기가 중복 생성되지 않게 한다.
	std::transform(OutKey.AssetPath.begin(), OutKey.AssetPath.end(), OutKey.AssetPath.begin(),
		[](unsigned char Character) { return static_cast<char>(std::tolower(Character)); });
	return true;
}

void FEditorManager::RegisterFactory(const FString& EditorType, FEditorFactory Factory)
{
	if (!bShuttingDown && !EditorType.empty() && Factory) Factories.Add(EditorType, Factory);
}

bool FEditorManager::OpenEditor(const FAssetEditorKey& Key)
{
	if (!bInitialized || bShuttingDown) return false;
	FAssetEditorKey Normalized;
	if (!NormalizeKey(Key, Normalized) || !Factories.Find(Normalized.EditorType)) return false;

	for (const FAssetEditorKey& Pending : PendingOpens)
	{
		if (Pending == Normalized) return true;
	}
	PendingOpens.Add(Normalized);
	return true;
}

FAssetEditor* FEditorManager::FindEditor(const FAssetEditorKey& Key) const
{
	FAssetEditorKey Normalized;
	if (!NormalizeKey(Key, Normalized)) return nullptr;
	for (const auto& Editor : Editors)
	{
		if (Editor->GetKey() == Normalized) return Editor.get();
	}
	return nullptr;
}

void FEditorManager::CreateEditor(const FAssetEditorKey& Key)
{
	if (FAssetEditor* Existing = FindEditor(Key))
	{
		Existing->Activate();
		return;
	}

	const FEditorFactory* Found = Factories.Find(Key.EditorType);
	if (!Found) return;
	// 생성 도중 다른 팩토리가 등록되어도 현재 콜백은 유효하게 유지한다.
	const FEditorFactory Factory = *Found;
	TUniquePtr<FAssetEditor> Editor = Factory(*this, Key);
	if (!Editor) return;
	if (&Editor->Manager != this || !(Editor->GetKey() == Key) || !Editor->Initialize())
	{
		Editor->Shutdown();
		HTR_LOG(Warning, "Asset editor initialization failed: {}", Key.AssetPath);
		return;
	}
	Editors.Add(std::move(Editor));
}

void FEditorManager::Tick(float DeltaTime)
{
	if (!bInitialized || bShuttingDown) return;
	LevelEditorTab->Tick(DeltaTime);
	for (auto& Editor : Editors)
	{
		if (!Editor->IsClosing()) Editor->Tick(DeltaTime);
	}
}

bool FEditorManager::IsLevelEditorInputEnabled() const
{
	// 메인 선택 탭과 분리된 에셋 패널의 키보드 포커스는 별개다.
	return bInitialized && !bShuttingDown && LevelEditorTab && LevelEditorTab->IsActive() && !HasFocusedAssetEditor();
}

bool FEditorManager::HasFocusedAssetEditor() const
{
	for (const auto& Editor : Editors)
	{
		if (!Editor->IsClosing() && Editor->IsFocused()) return true;
	}
	return false;
}

void FEditorManager::RenderPreview()
{
	if (!bInitialized || bShuttingDown) return;
	for (auto& Editor : Editors)
	{
		if (!Editor->IsClosing()) Editor->RenderPreview();
	}
}

void FEditorManager::DrawEditorUI()
{
	if (!bInitialized || bShuttingDown) return;
	TabContainer.OnRender();
	DrawCloseConfirmation();
}

void FEditorManager::DrawCloseConfirmation()
{
	if (!ClosePromptEditor)
	{
		for (auto& Editor : Editors)
		{
			if (Editor->GetCloseState() == EAssetEditorCloseState::AwaitingDecision)
			{
				ClosePromptEditor = Editor.get();
				bCloseActionFailed = false;
				break;
			}
		}
	}
	if (!ClosePromptEditor) return;

	ImGui::PushID(this);
	const char* PopupName = "Save changes?###CloseAssetEditor";
	if (!ImGui::IsPopupOpen(PopupName)) ImGui::OpenPopup(PopupName);
	if (ImGui::BeginPopupModal(PopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted(ClosePromptEditor->GetKey().AssetPath.c_str());
		if (!ClosePromptEditor->GetKey().SubAssetName.empty())
			ImGui::TextUnformatted(ClosePromptEditor->GetKey().SubAssetName.c_str());
		ImGui::TextUnformatted("Save changes before closing?");
		if (bCloseActionFailed) ImGui::TextUnformatted("The action failed. The editor remains open.");

		bool bResolved = ClosePromptEditor->GetCloseState() != EAssetEditorCloseState::AwaitingDecision;
		ImGui::BeginDisabled(!ClosePromptEditor->CanSave());
		if (ImGui::Button("Save"))
		{
			bResolved = ClosePromptEditor->SaveAndClose();
			bCloseActionFailed = !bResolved;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Discard"))
		{
			bResolved = ClosePromptEditor->DiscardAndClose();
			bCloseActionFailed = !bResolved;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ClosePromptEditor->CancelClose();
			bResolved = true;
		}

		if (bResolved)
		{
			ImGui::CloseCurrentPopup();
			ClosePromptEditor = nullptr;
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

void FEditorManager::LateTick()
{
	if (!bInitialized || bShuttingDown) return;
	for (int32 Index = Editors.Num() - 1; Index >= 0; --Index)
	{
		if (Editors[Index]->IsClosing())
		{
			if (ClosePromptEditor == Editors[Index].get()) ClosePromptEditor = nullptr;
			Editors[Index]->Shutdown();
			Editors.RemoveAt(Index);
		}
	}

	TArray<FAssetEditorKey> Requests = std::move(PendingOpens);
	PendingOpens = {};
	for (const FAssetEditorKey& Key : Requests) CreateEditor(Key);
}

void FEditorManager::Shutdown()
{
	if (bShuttingDown) return;
	bShuttingDown = true;
	ClosePromptEditor = nullptr;
	PendingOpens.Empty();
	for (auto& Editor : Editors) Editor->Shutdown();
	Editors.Empty();
	Factories.Reset();

	TabContainer.RemoveTab(LevelEditorTab.get());
	LevelEditorTab.reset();
	bInitialized = false;
}
