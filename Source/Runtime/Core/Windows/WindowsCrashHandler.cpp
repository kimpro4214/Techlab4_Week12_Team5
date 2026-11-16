#include "EnginePCH.h"        
#include "Core/Windows/WindowsCrashHandler.h"
#include <DbgHelp.h>                      
#pragma comment(lib, "Dbghelp.lib")       

namespace
{
	wchar_t DumpDirectory[MAX_PATH] = {};	// Dump 파일을 저장할 경로
	LONG IsHandlingCrash = 0;	// Crash Handling 중복 방지, 해당 Flag 여부로 Crash가 처리되었는지 체크
}

LONG WINAPI UnHandledExceptionHandler(EXCEPTION_POINTERS* ExceptPoint)
{
	if (InterlockedExchange(&IsHandlingCrash, 1) != 0)   // Handling이 이미 처리된 경우 다시 처리하지 않고 Execute Handler 반환
	{
		Sleep(INFINITE);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	SYSTEMTIME st;
	GetLocalTime(&st);


	IsHandlingCrash = 1;
	return EXCEPTION_EXECUTE_HANDLER;
}

void FCrashHandler::Install()
{

	wchar_t ExePath[MAX_PATH];
	GetModuleFileNameW(NULL, ExePath, MAX_PATH);	// Exe 파일 경로 가져옴
	std::filesystem::path Dir = std::filesystem::path(ExePath).parent_path() / L"Saved" / L"Crashes";	// Saved\Crashes\ 경로 하위에 저장되도록 Dir 설정
	std::filesystem::create_directories(Dir);	// 폴더 없을시 생성

	std::wstring DirString = Dir.wstring() + L"\\";
	wcscpy_s(DumpDirectory, DirString.c_str());		// Dump 경로 배열에 복사
	SetUnhandledExceptionFilter(&UnHandledExceptionHandler);	// Handler 세팅
}

