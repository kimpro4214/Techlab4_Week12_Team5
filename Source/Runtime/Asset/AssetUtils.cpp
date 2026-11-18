#include "EnginePCH.h"
#include "Asset/AssetUtils.h"

#include "Asset/Asset.h"
#include "Core/FileUtils.h"
#include "Render/Particle/ParticleModuleCollision.h"
#include "Serialization/AssetArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/Class.h"
#include "UObject/Property.h"
#include "UObject/Struct.h"
#include <cctype>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace AssetUtils
{
	FString MakeAssetKey(const FString& Path)
	{
		const FString Normalized = FFileUtils::NormalizePath(Path);
		if (GetAssetExtension(Normalized) != ".particle")
		{
			return Normalized;
		}

		// 에디터의 소문자 절대 경로도 디스크의 실제 표기로 변환해 같은 파티클 키를 사용한다.
		namespace fs = std::filesystem;
		std::error_code Error;
		const fs::path ProjectRoot = fs::canonical(".", Error);
		if (Error)
		{
			return Normalized;
		}

		const fs::path AssetPath(std::u8string(Normalized.begin(), Normalized.end()));
		const fs::path FilePath = fs::weakly_canonical(ProjectRoot / AssetPath, Error);
		if (Error)
		{
			return Normalized;
		}

		const fs::path RelativePath = FilePath.lexically_relative(ProjectRoot);
		const bool bInProject = !RelativePath.empty() && *RelativePath.begin() != "..";
		const std::u8string Key = (bInProject ? RelativePath : FilePath).generic_u8string();
		return FString(Key.begin(), Key.end());
	}

	FString GetAssetExtension(const FString& Path)
	{
		FString Extension = FFileUtils::GetExtension(Path);
		for (char& Character : Extension)
		{
			Character = static_cast<char>(std::tolower(static_cast<unsigned char>(Character)));
		}

		return Extension;
	}

	bool IsSerializedAssetExtension(const FString& Extension)
	{
		return Extension == ".particle" || Extension == ".asset";
	}

	void HashText(uint64& Hash, const FString& Text)
	{
		for (unsigned char Character : Text)
		{
			Hash = (Hash ^ Character) * 1099511628211ull;
		}

		// 문자열 경계를 섞어 속성 이름들을 단순 연결했을 때의 충돌을 줄인다.
		Hash = (Hash ^ 0xff) * 1099511628211ull;
	}

	void HashProperty(uint64& Hash, const FProperty& Property)
	{
		HashText(Hash, Property.Name);
		HashText(Hash, std::to_string(static_cast<int>(Property.Type)));

		if (Property.Enum)
		{
			HashText(Hash, Property.Enum->Info->Name);
			for (const FEnumEntry& Entry : Property.Enum->Info->Entries)
			{
				HashText(Hash, Entry.Name);
				HashText(Hash, std::to_string(Entry.Value));
			}
		}

		if (Property.Class)
		{
			HashText(Hash, Property.Class->Name);
		}

		if (Property.Array)
		{
			HashProperty(Hash, Property.Array->GetElementProperty());
		}

		if (const UScriptStruct* Struct = Property.GetScriptStruct())
		{
			for (const FProperty& Field : Struct->GetProperties())
			{
				HashProperty(Hash, Field);
			}
		}
	}

	uint64 GetSchema(UClass* Class, const FProperty* MissingProperty)
	{
		// 바이트를 기록하는 순서대로 검사해야 속성 추가·삭제·순서 변경을 감지할 수 있다.
		uint64 Hash = 14695981039346656037ull;
		for (; Class; Class = Class->Super)
		{
			HashText(Hash, Class->Name);
			for (const FProperty& Property : Class->GetProperties())
			{
				if (&Property != MissingProperty && !Property.HasAnyFlags(CPF_Transient))
				{
					HashProperty(Hash, Property);
				}
			}
		}

		return Hash;
	}

	void SerializeHeader(FArchive& Archive, FAssetFileHeader& Header)
	{
		// 필드 이름이 기록되지 않는 형식이므로 읽기와 쓰기 순서를 동일하게 유지한다.
		Archive << Header.Magic << Header.Version << Header.AssetVersion << Header.ObjectCount;
	}

	void SerializeObjectData(FArchive& Archive, FObjectData& Data)
	{
		Archive << Data.ClassName << Data.Name << Data.OuterIndex << Data.Schema;

		int32 Size = Data.Bytes.Num();
		Archive << Size;
		if (Archive.IsError() || Size < 0 || Size > FAssetArchive::MaxFileSize ||
			(Archive.IsLoading() && Size > Archive.TotalSize() - Archive.Tell()))
		{
			Archive.SetError();
			return;
		}

		if (Archive.IsLoading())
		{
			Data.Bytes.SetNum(Size);
		}

		if (Size > 0)
		{
			Archive.Serialize(Data.Bytes.GetData(), Size);
		}
	}

	using FObjectNamesByOuter = std::unordered_map<int32, std::unordered_set<FName, FNameHash>>;

	static bool TrackObjectName(const FObjectData& Data, FObjectNamesByOuter& NamesByOuter, FString& OutError)
	{
		if (Data.OuterIndex < 0 || Data.Name == NAME_None ||
			NamesByOuter[Data.OuterIndex].insert(Data.Name).second)
		{
			return true;
		}

		OutError = "Duplicate asset object name '" + Data.Name.ToString() +
			"' under outer record " + std::to_string(Data.OuterIndex);
		return false;
	}

	bool BuildObjectRecords(UAsset* Asset, TArray<FObjectData>& Records, FString& OutError)
	{
		TArray<UObject*> Objects;
		Objects.Add(Asset);
		FObjectNamesByOuter NamesByOuter;

		// 속성을 저장하다 새 내부 객체를 만나면 Objects에 추가되므로 Num()도 함께 늘어난다.
		int64 PayloadSize = 0;
		for (int32 Index = 0; Index < Objects.Num(); ++Index)
		{
			UObject* Object = Objects[Index];
			FObjectData Data;
			Data.ClassName = Object->GetClass()->Name;
			Data.Name = Index == 0 ? NAME_None : Object->GetFName();
			Data.OuterIndex = Index == 0 ? -1 : Objects.Find(Object->GetOuter());
			Data.Schema = GetSchema(Object->GetClass());
			if (!TrackObjectName(Data, NamesByOuter, OutError))
			{
				return false;
			}

			FMemoryWriter Writer(Data.Bytes);
			FAssetArchive Archive(Writer, Objects);
			Object->Serialize(Archive);

			PayloadSize += Data.Bytes.Num();
			if (Archive.IsError() || PayloadSize > FAssetArchive::MaxFileSize)
			{
				OutError = "Cannot serialize asset objects or references";
				return false;
			}

			Records.Add(std::move(Data));
		}

		return true;
	}

	bool WriteAssetBytes(UAsset* Asset, TArray<FObjectData>& Records, TArray<uint8>& OutBytes, FString& OutError)
	{
		FAssetFileHeader Header;
		Header.AssetVersion = Asset->GetAssetVersion();
		Header.ObjectCount = Records.Num();

		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes);
		SerializeHeader(Writer, Header);
		for (FObjectData& Data : Records)
		{
			SerializeObjectData(Writer, Data);
		}

		if (Writer.IsError() || Bytes.Num() > FAssetArchive::MaxFileSize)
		{
			OutError = "Asset file exceeds the size limit";
			return false;
		}

		// 전체 기록이 성공한 경우에만 호출자의 결과를 교체한다.
		OutBytes = std::move(Bytes);
		return true;
	}

	bool ReadFileHeader(FArchive& Archive, FAssetFileHeader& Header, FString& OutError)
	{
		SerializeHeader(Archive, Header);
		if (Archive.IsError() || Header.Magic != AssetMagic ||
			Header.ObjectCount <= 0 || Header.ObjectCount > FAssetArchive::MaxObjects)
		{
			return false;
		}

		if (Header.Version != FileVersion)
		{
			OutError = "Unsupported binary asset file version";
			return false;
		}

		return true;
	}

	bool ValidateObjectRecord(FObjectData& Data, int32 Index, UClass* ExpectedClass, FString& OutError)
	{
		Data.Class = FindClass(Data.ClassName);
		if (!Data.Class || !Data.Class->Constructor)
		{
			OutError = "Unknown or non-constructible asset object class: " + Data.ClassName;
			return false;
		}

		if (Data.Schema != GetSchema(Data.Class))
		{
			const FProperty* Damping = Data.Class == UParticleModuleCollision::StaticClass()
				? Data.Class->FindProperty("DampingFactor") : nullptr;
			if (!Damping || Data.Schema != GetSchema(Data.Class, Damping))
			{
				OutError = "Asset property schema has changed: " + Data.ClassName;
				return false;
			}

			// 기존 Collision 모듈은 첫 필드인 DampingFactor가 없으므로 생성자 기본값을 사용한다.
			Data.Bytes.Insert(0, 0);
		}

		if (Index == 0)
		{
			const bool bMatchesExpectedClass = !ExpectedClass || Data.Class->IsChildOf(ExpectedClass);
			return Data.Class->IsChildOf(UAsset::StaticClass()) && Data.OuterIndex == -1 && bMatchesExpectedClass;
		}

		// 내부 객체의 부모 ID는 반드시 앞에 있어야 한다. 외부 애셋은 이 테이블에서 생성하지 않는다.
		return Data.OuterIndex >= 0 && Data.OuterIndex < Index && !Data.Class->IsChildOf(UAsset::StaticClass());
	}

	bool ReadObjectRecords(FArchive& Archive, int32 Count, UClass* ExpectedClass,
		TArray<FObjectData>& Records, FString& OutError)
	{
		FObjectNamesByOuter NamesByOuter;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FObjectData Data;
			SerializeObjectData(Archive, Data);
			if (Archive.IsError() || !ValidateObjectRecord(Data, Index, ExpectedClass, OutError) ||
				!TrackObjectName(Data, NamesByOuter, OutError))
			{
				return false;
			}

			Records.Add(std::move(Data));
		}

		// 잘린 파일뿐 아니라 마지막 객체 뒤에 알 수 없는 데이터가 남은 파일도 거부한다.
		return Archive.Tell() == Archive.TotalSize();
	}

	TUniquePtr<UAsset> CreateAssetObjects(const TArray<FObjectData>& Records, uint32 AssetVersion,
		TArray<UObject*>& Objects, FString& OutError)
	{
		const FObjectData& Root = Records[0];
		TUniquePtr<UAsset> Asset(Cast<UAsset>(NewObject<UObject>(nullptr, Root.Class, Root.Name)));
		Objects.Add(Asset.get());

		if (Asset->GetAssetVersion() != AssetVersion)
		{
			OutError = "Unsupported asset serialization version";
			return nullptr;
		}

		// 속성을 읽기 전에 모든 객체를 생성해야 공유 참조와 순환 참조를 복원할 수 있다.
		for (int32 Index = 1; Index < Records.Num(); ++Index)
		{
			const FObjectData& Data = Records[Index];
			UObject* Outer = Objects[Data.OuterIndex];
			Objects.Add(NewObject<UObject>(Outer, Data.Class, Data.Name));
		}

		return Asset;
	}

	bool RestoreObjectData(const TArray<FObjectData>& Records, TArray<UObject*>& Objects)
	{
		for (int32 Index = 0; Index < Records.Num(); ++Index)
		{
			FMemoryReader Reader(Records[Index].Bytes);
			FAssetArchive Archive(Reader, Objects);
			Objects[Index]->Serialize(Archive);

			if (Archive.IsError() || Archive.Tell() != Archive.TotalSize())
			{
				return false;
			}
		}

		return true;
	}

}
