#pragma once

#include "Core/Types.h"
#include "Core/EngineString.h"
#include "Containers/Array.h"
#include <limits>

// 파일 시스템과의 경계. 네이티브 경로 타입은 구현 안에서만 사용한다.
class FFileUtils
{
public:
	static FString NormalizePath(const FString& Path);
	static FString GetExtension(const FString& Path);

	static bool ReadAllBytes(const FString& Path, TArray<uint8>& OutBytes, FString& OutError,
		int64 MaxSize = std::numeric_limits<int32>::max());
	static bool WriteAllBytesAtomic(const FString& Path, const TArray<uint8>& Bytes, FString& OutError);
};
