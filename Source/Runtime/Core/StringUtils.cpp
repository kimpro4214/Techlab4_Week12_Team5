#include "EnginePCH.h"
#include "Core/StringUtils.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <limits>
#include <utility>

bool StringUtils::TryUtf8ToWide(const FString& Input, FWideString& Output)
{
	if (Input.empty())
	{
		Output.clear();
		return true;
	}

	if (Input.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
	{
		return false;
	}

	// 입력은 바이트 수, 출력은 UTF-16 코드 단위 수다. 잘못된 입력은 치환하지 않는다.
	const int InputLength = static_cast<int>(Input.size());
	const int Length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		Input.data(), InputLength, nullptr, 0);
	if (Length == 0)
	{
		return false;
	}

	FWideString Converted(Length, L'\0');
	const int Written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		Input.data(), InputLength, Converted.data(), Length);
	if (Written != Length)
	{
		return false;
	}

	Output = std::move(Converted);
	return true;
}

bool StringUtils::TryWideToUtf8(const FWideString& Input, FString& Output)
{
	if (Input.empty())
	{
		Output.clear();
		return true;
	}

	if (Input.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
	{
		return false;
	}

	// UTF-16 코드 단위 수와 UTF-8 바이트 수를 구분하고, 서로게이트 오류를 거부한다.
	const int InputLength = static_cast<int>(Input.size());
	const int Length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
		Input.data(), InputLength, nullptr, 0, nullptr, nullptr);
	if (Length == 0)
	{
		return false;
	}

	FString Converted(Length, '\0');
	const int Written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
		Input.data(), InputLength, Converted.data(), Length, nullptr, nullptr);
	if (Written != Length)
	{
		return false;
	}

	Output = std::move(Converted);
	return true;
}
