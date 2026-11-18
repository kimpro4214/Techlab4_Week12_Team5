#pragma once

#include "Core/Types.h"
#include "Core/EngineString.h"
#include "Containers/Array.h"
#include "Containers/StringView.h"
#include <filesystem>
#include <limits>

// 엔진의 UTF-8 경로와 파일 시스템의 네이티브 경로를 변환한다.
class FFileUtils
{
public:
	static std::filesystem::path ToNativePath(FStringView Path);
	static FString ToUTF8String(const std::filesystem::path& Path);

	static FString NormalizePath(const FString& Path);
	static FString GetExtension(const FString& Path);

	static bool ReadAllBytes(const FString& Path, TArray<uint8>& OutBytes, FString& OutError,
		int64 MaxSize = std::numeric_limits<int32>::max());
	static bool WriteAllBytesAtomic(const FString& Path, const TArray<uint8>& Bytes, FString& OutError);
};
