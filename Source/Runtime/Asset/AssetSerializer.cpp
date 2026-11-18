#include "EnginePCH.h"
#include "Asset/AssetSerializer.h"

#include "Asset/Asset.h"
#include "Asset/AssetUtils.h"
#include "Core/FileUtils.h"
#include "Serialization/AssetArchive.h"
#include "Serialization/MemoryReader.h"

bool FAssetSerializer::SaveToBytes(UAsset* Asset, TArray<uint8>& OutBytes, FString& OutError)
{
	OutError.clear();
	OutBytes.Reset();

	if (!Asset || !Asset->ValidateAsset(OutError))
	{
		if (OutError.empty())
		{
			OutError = "Invalid asset";
		}

		return false;
	}

	TArray<AssetUtils::FObjectData> Records;
	if (!AssetUtils::BuildObjectRecords(Asset, Records, OutError))
	{
		return false;
	}

	return AssetUtils::WriteAssetBytes(Asset, Records, OutBytes, OutError);
}

UAsset* FAssetSerializer::LoadFromBytes(const TArray<uint8>& Bytes, UClass* ExpectedClass, FString& OutError) try
{
	OutError = "Invalid or unsupported binary asset";
	if (Bytes.Num() < AssetUtils::HeaderSize || Bytes.Num() > FAssetArchive::MaxFileSize)
	{
		return nullptr;
	}

	FMemoryReader Reader(Bytes);
	AssetUtils::FAssetFileHeader Header;
	TArray<AssetUtils::FObjectData> Records;
	if (!AssetUtils::ReadFileHeader(Reader, Header, OutError) ||
		!AssetUtils::ReadObjectRecords(Reader, Header.ObjectCount, ExpectedClass, Records, OutError))
	{
		return nullptr;
	}

	TArray<UObject*> Objects;
	TUniquePtr<UAsset> Asset = AssetUtils::CreateAssetObjects(Records, Header.AssetVersion, Objects, OutError);
	if (!Asset || !AssetUtils::RestoreObjectData(Records, Objects) || !Asset->ValidateAsset(OutError))
	{
		return nullptr;
	}

	Asset->PostLoadAsset();
	OutError.clear();
	return Asset.release();
}
catch (const std::exception& Error)
{
	OutError = FString("Invalid asset data: ") + Error.what();
	return nullptr;
}

bool FAssetSerializer::Save(UAsset* Asset, const FString& Path, FString& OutError) try
{
	TArray<uint8> Bytes;
	if (!SaveToBytes(Asset, Bytes, OutError))
	{
		return false;
	}

	return FFileUtils::WriteAllBytesAtomic(Path, Bytes, OutError);
}
catch (const std::exception& Error)
{
	OutError = FString("Cannot save asset: ") + Error.what();
	return false;
}

UAsset* FAssetSerializer::Load(const FString& Path, UClass* ExpectedClass, FString& OutError) try
{
	TArray<uint8> Bytes;
	if (!FFileUtils::ReadAllBytes(Path, Bytes, OutError, FAssetArchive::MaxFileSize))
	{
		return nullptr;
	}

	TUniquePtr<UAsset> Asset(LoadFromBytes(Bytes, ExpectedClass, OutError));
	if (Asset)
	{
		Asset->SetPath(AssetUtils::MakeAssetKey(Path));
	}

	return Asset.release();
}
catch (const std::exception& Error)
{
	OutError = FString("Cannot load asset: ") + Error.what();
	return nullptr;
}
