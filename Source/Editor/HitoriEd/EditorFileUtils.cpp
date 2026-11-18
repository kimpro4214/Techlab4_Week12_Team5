#include "EnginePCH.h"
#include "Editor/HitoriEd/EditorFileUtils.h"

#include "Core/EngineLog.h"
#include "Core/FileUtils.h"
#include <commdlg.h>
#include "Serialization/JsonArchive.h"
 
FString FEditorFileUtils::CurrentScenePath = "";

bool FEditorFileUtils::NewScene(UWorld* World)
{
	if (!World)
		return false;

	World->ClearWorld();

	CurrentScenePath.clear();

	return true;
}

bool FEditorFileUtils::SaveScene(UWorld* World)
{
	if (!World)
		return false;

	if (CurrentScenePath.empty())
	{
		return SaveSceneAs(World);
	}

	if (!FJsonArchive::SaveWorld(World, CurrentScenePath))
	{
		HTR_LOG(Error, "Failed to save scene: {}", CurrentScenePath);

		MessageBoxW(
			GetActiveWindow(),
			L"씬을 저장하지 못했습니다.\n경로와 쓰기 권한을 확인하세요.",
			L"저장 실패",
			MB_OK | MB_ICONERROR
		);
		return false;
	}
	HTR_LOG(Info, "Save Scene : {}", CurrentScenePath);

	return true;
}

bool FEditorFileUtils::SaveSceneAs(UWorld* World)
{
	if (!World)
		return false;

	FString FilePath = OpenSaveSceneDialog();

	if (FilePath.empty())
		return false;

	if (!FJsonArchive::SaveWorld(World, FilePath))
	{
		HTR_LOG(Error, "Failed to save scene: {}", FilePath);

		MessageBoxW(
			GetActiveWindow(),
			L"씬을 저장하지 못했습니다.\n경로와 쓰기 권한을 확인하세요.",
			L"저장 실패",
			MB_OK | MB_ICONERROR
		);

		return false;
	}

	CurrentScenePath = FilePath;

	HTR_LOG(Info, "Save Scene As : {}", CurrentScenePath);

	return true;
}

bool FEditorFileUtils::LoadScene(UWorld* World)
{
	if (!World)
		return false;

	FString FilePath = OpenLoadSceneDialog();

	// 파일 선택 취소
	if (FilePath.empty())
		return false;

	if (!FJsonArchive::LoadWorld(World, FilePath))
		return false;

	// 이제 이 파일이 현재 Scene
	CurrentScenePath = FilePath;

	HTR_LOG(Info, "Load Scene : {}", CurrentScenePath);

	return true;
}

FString FEditorFileUtils::OpenSaveSceneDialog()
{
	wchar_t FilePath[MAX_PATH] = {};

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = FilePath;
	ofn.nMaxFile = MAX_PATH;

	ofn.lpstrFilter = L"Scene Files (*.scene)\0*.scene\0";
	ofn.lpstrDefExt = L"scene";
	ofn.lpstrInitialDir = L"Scene/";

	ofn.Flags =
		OFN_PATHMUSTEXIST |
		OFN_OVERWRITEPROMPT |
		OFN_NOCHANGEDIR;

	if (GetSaveFileNameW(&ofn))
	{
		return FFileUtils::ToUTF8String(std::filesystem::path(FilePath).lexically_normal());
	}

	return FString();
}

FString FEditorFileUtils::OpenLoadSceneDialog()
{
	wchar_t FilePath[MAX_PATH] = {};

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = nullptr;

	ofn.lpstrFile = FilePath;
	ofn.nMaxFile = MAX_PATH;

	ofn.lpstrFilter =
		L"Scene Files (*.scene)\0*.scene\0"
		L"All Files (*.*)\0*.*\0";

	ofn.lpstrDefExt = L"scene";
	ofn.lpstrInitialDir = L"Scene/";

	ofn.Flags =
		OFN_FILEMUSTEXIST |
		OFN_PATHMUSTEXIST |
		OFN_NOCHANGEDIR;

	if (GetOpenFileNameW(&ofn))
	{
		return FFileUtils::ToUTF8String(std::filesystem::path(FilePath).lexically_normal());
	}

	return FString();
}


