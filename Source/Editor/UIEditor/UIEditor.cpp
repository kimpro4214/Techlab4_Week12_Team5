#include "EnginePCH.h"
#include "Editor/UIEditor/UIEditor.h"
#include "Editor/UIEditor/UIEditorTab.h"
#include "Render/RenderCommand.h"
#include "UI/UIRenderer.h"
#include "UI/UserWidget.h"
#include "UI/WidgetSerializer.h"

#include <fstream>

namespace
{
	bool WriteAsset(const FString& Path, const FString& Data)
	{
		// 저장 실패 시 기존 파일을 보존하도록 같은 디렉터리에서 교체한다.
		const fs::path Target(Path);
		const fs::path Temporary = Target.wstring() + L".ui-editor.tmp";
		std::ofstream Out(Temporary, std::ios::binary | std::ios::trunc);
		Out << Data;
		Out.close();
		if (!Out.fail() && MoveFileExW(Temporary.c_str(), Target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			FWidgetAssetCache::Invalidate(Path);
			return true;
		}

		std::error_code Error;
		fs::remove(Temporary, Error);
		HTR_LOG(Error, "Cannot save widget asset: {}", Path);
		return false;
	}
}

FUIEditor::FUIEditor(FEditorManager& InManager, const FAssetEditorKey& InKey, FUIRenderer* InRenderer)
	: FAssetEditor(InManager, InKey), Renderer(InRenderer)
{
}

FUIEditor::~FUIEditor()
{
	Shutdown();
}

TUniquePtr<IEditorTab> FUIEditor::CreateTab()
{
	return MakeUnique<FUIEditorTab>(*this);
}

bool FUIEditor::CreateAssetFile(const FString& Path)
{
	// 신규 생성 요청으로 기존 에셋을 덮어쓰지 않는다.
	std::error_code Error;
	if (fs::exists(Path, Error) || Error) return false;
	TUniquePtr<UCanvasPanel> NewRoot(NewObject<UCanvasPanel>());
	NewRoot->WidgetName = "Root";
	NewRoot->Anchors = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
	NewRoot->Offsets = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	NewRoot->Visibility = 0;
	json File = { {"Version", 1}, {"ParentClass", "UUserWidget"},
		{"DesignSize", {1920, 1080}}, {"Root", FWidgetSerializer::SaveWidget(NewRoot.get())} };
	return WriteAsset(Path, File.dump(2));
}

bool FUIEditor::InitializeResources()
{
	std::ifstream In(GetKey().AssetPath);
	json File = In ? json::parse(In, nullptr, false) : json();
	if (File.is_discarded() || !File.is_object() || !File.contains("Root")) return false;
	if (!RestoreSnapshot(File.dump(), true)) return false;
	LastSnapshot = Snapshot();
	SavedSnapshot = LastSnapshot;
	return true;
}

void FUIEditor::ShutdownResources()
{
	Selected = nullptr;
	PendingReparent = {};
	delete Root;
	Root = nullptr;
	PreviewTargets.Reset();
	DrawList.Reset();
	UndoStack.Reset();
	RedoStack.Reset();
	BeginUIFrame();
}

void FUIEditor::BeginUIFrame()
{
	bFocused = false;
	bCanvasVisible = false;
}

void FUIEditor::EndUIFrame(bool bEditing)
{
	if (PendingReparent.Widget)
	{
		ReparentWidget(PendingReparent.Widget, PendingReparent.NewParent);
		PendingReparent = {};
	}
	if (!bCanvasVisible) DragMode = EDragMode::None;
	CommitUndoIfChanged(bEditing);
}

bool FUIEditor::IsDirty() const
{
	return Root && Snapshot() != SavedSnapshot;
}

FString FUIEditor::Snapshot() const
{
	// 부모 클래스도 편집 이력에 포함한다. 저장 기준은 기존처럼 1920x1080을 유지한다.
	json File = { {"Version", 1}, {"ParentClass", ParentClassName},
		{"DesignSize", {1920, 1080}}, {"Root", FWidgetSerializer::SaveWidget(Root)} };
	return File.dump();
}

bool FUIEditor::RestoreSnapshot(const FString& Data, bool bRestorePreviewSize)
{
	try
	{
		const json File = json::parse(Data);
		const FString Parent = File.value("ParentClass", std::string("UUserWidget"));
		FVector2 Size(1920.0f, 1080.0f);
		if (File.contains("DesignSize"))
		{
			Size = FVector2(File.at("DesignSize").at(0).get<float>(), File.at("DesignSize").at(1).get<float>());
			if (!std::isfinite(Size.X) || !std::isfinite(Size.Y) || Size.X <= 0 || Size.Y <= 0 || Size.X > 16384 || Size.Y > 16384)
				return false;
		}
		TUniquePtr<UWidget> Loaded(FWidgetSerializer::LoadWidget(File.at("Root")));
		UPanelWidget* NewRoot = Cast<UPanelWidget>(Loaded.get());
		if (!NewRoot) return false;

		const FString SelectedName = Selected ? Selected->WidgetName : FString();
		delete Root;
		Root = NewRoot;
		Loaded.release();
		Selected = Root->WidgetName == SelectedName ? Root : Root->FindWidget(SelectedName);
		ParentClassName = Parent;
		if (bRestorePreviewSize) PreviewSize = Size;
		PendingReparent = {};
		DragMode = EDragMode::None;
		return true;
	}
	catch (const json::exception&)
	{
		HTR_LOG(Error, "Invalid widget asset: {}", GetKey().AssetPath);
		return false;
	}
}

void FUIEditor::CommitUndoIfChanged(bool bEditing)
{
	if (!Root || bEditing || DragMode != EDragMode::None) return;
	FString Now = Snapshot();
	if (Now == LastSnapshot) return;
	UndoStack.Add(LastSnapshot);
	RedoStack.Reset();
	LastSnapshot = std::move(Now);
}

void FUIEditor::Undo()
{
	CommitUndoIfChanged();
	if (UndoStack.IsEmpty() || !RestoreSnapshot(UndoStack.Last())) return;
	RedoStack.Add(LastSnapshot);
	LastSnapshot = UndoStack.Last();
	UndoStack.RemoveLast();
}

void FUIEditor::Redo()
{
	CommitUndoIfChanged();
	if (RedoStack.IsEmpty() || !RestoreSnapshot(RedoStack.Last())) return;
	UndoStack.Add(LastSnapshot);
	LastSnapshot = RedoStack.Last();
	RedoStack.RemoveLast();
}

bool FUIEditor::Save()
{
	if (!CanSave()) return false;
	CommitUndoIfChanged();
	const FString Data = Snapshot();
	if (!WriteAsset(GetKey().AssetPath, json::parse(Data).dump(2))) return false;
	SavedSnapshot = Data;
	return true;
}

bool FUIEditor::DiscardChanges()
{
	return !Root || RestoreSnapshot(SavedSnapshot);
}

bool FUIEditor::ContainsWidget(const UWidget* Widget) const
{
	if (!Root || !Widget) return false;
	// 다른 에디터에서 전달되거나 이미 삭제된 포인터를 역참조하지 않는다.
	TArray<const UWidget*> Pending{ Root };
	while (!Pending.IsEmpty())
	{
		const UWidget* Current = Pending.Last();
		Pending.RemoveLast();
		if (Current == Widget) return true;
		if (const UPanelWidget* Panel = Cast<UPanelWidget>(Current))
			for (const UWidget* Child : Panel->GetChildren()) Pending.Add(Child);
	}
	return false;
}

void FUIEditor::SelectWidget(UWidget* Widget)
{
	Selected = ContainsWidget(Widget) ? Widget : nullptr;
}

UWidget* FUIEditor::AddWidget(UClass* Class)
{
	if (!Root || !Class || !Class->Constructor || !Class->IsChildOf(UWidget::StaticClass()) ||
		Class == UPanelWidget::StaticClass() || Class->IsChildOf(UUserWidget::StaticClass())) return nullptr;
	UPanelWidget* Target = Cast<UPanelWidget>(Selected);
	if (!Target) Target = Selected && Selected->GetParent() ? Selected->GetParent() : Root;
	UWidget* Widget = NewObject<UWidget>(nullptr, Class);
	Widget->WidgetName = Class->Name.substr(1) + std::to_string(Widget->GetUUID());
	Widget->Offsets.X = 40.0f;
	Widget->Offsets.Y = 40.0f;
	Target->AddChild(Widget);
	Selected = Widget;
	return Widget;
}

bool FUIEditor::ReparentWidget(UWidget* Widget, UPanelWidget* NewParent)
{
	if (Widget == Root || !ContainsWidget(Widget) || !ContainsWidget(NewParent)) return false;
	for (const UWidget* It = NewParent; It; It = It->GetParent())
		if (It == Widget) return false;
	NewParent->AddChild(Widget);
	return true;
}

UWidget* FUIEditor::PickWidget(UWidget* Widget, float X, float Y) const
{
	if (!Widget || Widget->IsCollapsed() || Widget->Visibility == static_cast<int32>(EUIVisibility::Hidden))
		return nullptr;

	if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		TArray<UWidget*> Sorted;
		Panel->GetSortedChildren(Sorted);
		for (auto It = Sorted.rbegin(); It != Sorted.rend(); ++It)
			if (UWidget* Hit = PickWidget(*It, X, Y))
				return Hit;
	}
	// 루트는 빈 곳 클릭으로 선택되지 않게 한다.
	return (Widget != Root && Widget->GetCachedRect().Contains(X, Y)) ? Widget : nullptr;
}

void FUIEditor::ApplyDrag(float DX, float DY)
{
	FVector4 O = DragStartOffsets;
	const FVector4& A = Selected->Anchors;
	const bool bStretchX = A.X != A.Z;
	const bool bStretchY = A.Y != A.W;

	if (DragMode == EDragMode::Move)
	{
		O.X += DX; if (bStretchX) O.Z -= DX;
		O.Y += DY; if (bStretchY) O.W -= DY;
	}
	else
	{
		// 늘어나는 축은 오른쪽/아래 여백이 줄고, 점 앵커 축은 폭/높이가 는다.
		O.Z += bStretchX ? -DX : DX;
		O.W += bStretchY ? -DY : DY;
	}
	Selected->Offsets = O;
}

// 앵커 프리셋을 바꿔도 화면 위 위치가 그대로 남도록 오프셋을 다시 계산한다.

void FUIEditor::SetAnchorsKeepRect(UWidget* Widget, const FVector4& NewAnchors)
{
	const FUIRect R = Widget->GetCachedRect();
	const FUIRect P = Widget->GetParent() ? Widget->GetParent()->GetCachedRect()
		: FUIRect{ 0.0f, 0.0f, PreviewSize.X, PreviewSize.Y };

	Widget->Anchors = NewAnchors;
	const float A0X = P.X + P.W * NewAnchors.X, A1X = P.X + P.W * NewAnchors.Z;
	const float A0Y = P.Y + P.H * NewAnchors.Y, A1Y = P.Y + P.H * NewAnchors.W;

	if (NewAnchors.X == NewAnchors.Z) { Widget->Offsets.X = R.X - A0X + R.W * Widget->AlignmentX; Widget->Offsets.Z = R.W; }
	else { Widget->Offsets.X = R.X - A0X; Widget->Offsets.Z = A1X - (R.X + R.W); }
	if (NewAnchors.Y == NewAnchors.W) { Widget->Offsets.Y = R.Y - A0Y + R.H * Widget->AlignmentY; Widget->Offsets.W = R.H; }
	else { Widget->Offsets.Y = R.Y - A0Y; Widget->Offsets.W = A1Y - (R.Y + R.H); }
}

void FUIEditor::DeleteSelected()
{
	if (!Selected || Selected == Root) return;
	UWidget* Target = Selected;
	PendingReparent = {};
	DragMode = EDragMode::None;
	Selected = Target->GetParent();
	Target->RemoveFromParent();
	delete Target;   // 패널이면 자식도 함께 지워진다
}

void FUIEditor::RenderPreview()
{
	if (!Root || !bCanvasVisible || !Renderer || IsClosing()) return;

	const uint32 Width = static_cast<uint32>(PreviewSize.X);
	const uint32 Height = static_cast<uint32>(PreviewSize.Y);
	if (!PreviewTargets.Resize(Width, Height)) return;

	DrawList.Reset();
	Root->Arrange({ FUIRect{ 0.0f, 0.0f, PreviewSize.X, PreviewSize.Y }, 1.0f });
	Root->Paint(DrawList, 1.0f);

	FTexture2D* Color = PreviewTargets.SceneColor.get();
	RenderCommand::BeginRenderPass(Color, PreviewTargets.Depth.get(), Width, Height);
	const float Clear[4] = { 0.10f, 0.12f, 0.15f, 1.0f };
	RenderCommand::ClearRenderTarget(Color, Clear);
	Renderer->Render(DrawList, Width, Height);
	RenderCommand::EndRenderPass();
}
