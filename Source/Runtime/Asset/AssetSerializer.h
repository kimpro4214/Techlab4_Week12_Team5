#pragma once

#include "Core/Types.h"
#include "Core/EngineString.h"
#include "Containers/Array.h"

class UAsset;
class UClass;

// UObject의 Serialize(Record)를 공통 바이너리 파일로 저장한다.
// 반환된 애셋은 호출자가 소유하며, AssetManager 등록 전까지 직접 삭제할 수 있다.
class FAssetSerializer
{
public:
	static bool SaveToBytes(UAsset* Asset, TArray<uint8>& OutBytes, FString& OutError);
	static UAsset* LoadFromBytes(const TArray<uint8>& Bytes, UClass* ExpectedClass, FString& OutError);

	static bool Save(UAsset* Asset, const FString& Path, FString& OutError);
	static UAsset* Load(const FString& Path, UClass* ExpectedClass, FString& OutError);
};
