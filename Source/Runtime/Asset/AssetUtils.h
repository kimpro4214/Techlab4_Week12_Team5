#pragma once

#include "Core/Types.h"
#include "Core/EngineString.h"
#include "Core/NameTypes.h"
#include "Containers/Array.h"

class FArchive;
class UAsset;
class UClass;
class UObject;
struct FProperty;

namespace AssetUtils
{
	// 4문자를 little endian 바이트 순서로 배치한다. 배열 참조로 문자열 길이도 제한한다.
	constexpr uint32 MakeMagicNumber(const char (&Code)[5])
	{
		uint32 Magic = 0;
		for (uint32 Index = 0; Index < 4; ++Index)
		{
			Magic |= static_cast<uint32>(static_cast<uint8>(Code[Index])) << (Index * 8);
		}

		return Magic;
	}

	constexpr uint32 AssetMagic = MakeMagicNumber("HAST");
	constexpr uint32 FileVersion = 1;
	constexpr int32 HeaderSize = 16;

	struct FAssetFileHeader
	{
		uint32 Magic = AssetMagic;
		uint32 Version = FileVersion;
		uint32 AssetVersion = 0;
		int32 ObjectCount = 0;
	};

	struct FObjectData
	{
		FString ClassName;
		FName Name;
		int32 OuterIndex = -1;
		uint64 Schema = 0;
		TArray<uint8> Bytes;

		// 파일에서 찾은 클래스를 보관한다. 이 포인터 자체는 저장하지 않는다.
		UClass* Class = nullptr;
	};

	FString MakeAssetKey(const FString& Path);
	FString GetAssetExtension(const FString& Path);
	bool IsSerializedAssetExtension(const FString& Extension);

	void HashText(uint64& Hash, const FString& Text);
	void HashProperty(uint64& Hash, const FProperty& Property);
	uint64 GetSchema(UClass* Class);

	void SerializeHeader(FArchive& Archive, FAssetFileHeader& Header);
	void SerializeObjectData(FArchive& Archive, FObjectData& Data);
	bool BuildObjectRecords(UAsset* Asset, TArray<FObjectData>& Records, FString& OutError);
	bool WriteAssetBytes(UAsset* Asset, TArray<FObjectData>& Records, TArray<uint8>& OutBytes, FString& OutError);

	bool ReadFileHeader(FArchive& Archive, FAssetFileHeader& Header, FString& OutError);
	bool ValidateObjectRecord(FObjectData& Data, int32 Index, UClass* ExpectedClass, FString& OutError);
	bool ReadObjectRecords(FArchive& Archive, int32 Count, UClass* ExpectedClass,
		TArray<FObjectData>& Records, FString& OutError);
	TUniquePtr<UAsset> CreateAssetObjects(const TArray<FObjectData>& Records, uint32 AssetVersion,
		TArray<UObject*>& Objects, FString& OutError);
	bool RestoreObjectData(const TArray<FObjectData>& Records, TArray<UObject*>& Objects);
}
