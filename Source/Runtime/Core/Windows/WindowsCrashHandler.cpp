#include "EnginePCH.h"        
#include "Core/Windows/WindowsCrashHandler.h"
#include <DbgHelp.h>                      
#pragma comment(lib, "Dbghelp.lib")       

namespace
{
	wchar_t DumpDirectory[MAX_PATH] = {};	// Dump 파일을 저장할 경로
	LONG IsHandlingCrash = 0;	// Crash Handling 중복 방지, 해당 Flag 여부로 Crash가 처리되었는지 체크

	LONG WINAPI UnHandledExceptionHandler(EXCEPTION_POINTERS* ExceptPoint)
	{
		if (InterlockedExchange(&IsHandlingCrash, 1) != 0)   // Handling이 이미 처리된 경우 다시 처리하지 않고 Execute Handler 반환
		{
			Sleep(INFINITE);
			return EXCEPTION_EXECUTE_HANDLER;
		}

		wchar_t DumpFilePath[MAX_PATH];
		SYSTEMTIME st;
		GetLocalTime(&st);
		swprintf_s(DumpFilePath, L"%sCrash_%04d%02d%02d_%02d%02d%02d.dmp", DumpDirectory, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);	// LocalTime 바탕으로 DumpFilePath 결정

		HANDLE hFile = CreateFileW(DumpFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);	// File Handle 생성, CreateAlways로 덮어씌우기, 작업중에 다른 프로세스 읽기만 가능


		if (hFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION mdei;
			mdei.ThreadId = GetCurrentThreadId();	// Thread ID
			mdei.ExceptionPointers = ExceptPoint;	// Parameter로 받은 Pointer
			mdei.ClientPointers = FALSE;			// ExceptionPointer가 어느 프로세스 메모리를 가리키는지 체크, 크래쉬 난 프로세스에서 직접 덤프쓰면 False, 크래쉬 프로세스가 따로 존재하면 True

			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, (ExceptPoint != nullptr) ? &mdei : nullptr, nullptr, nullptr);	// MiniDump에 기록

			CloseHandle(hFile);
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}
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

