#include "EnginePCH.h"
#include "Serialization/AssetArchive.h"

#include "Asset/Asset.h"
#include "Asset/AssetManager.h"

FAssetArchive::FAssetArchive(FArchive& InInner, TArray<UObject*>& InObjects)
	: Inner(InInner), Objects(InObjects)
{
	SetIsLoading(Inner.IsLoading());
	SetIsSaving(Inner.IsSaving());
	SetIsPersistent(true);
	SetPortFlags(EPropertyPortFlags::PPF_Asset);
}

void FAssetArchive::Serialize(void* Data, int64 Size)
{
	if (IsError())
	{
		return;
	}

	if (Size < 0 || Tell() < 0 || Size > MaxFileSize - Tell())
	{
		SetError();
		return;
	}

	Inner.Serialize(Data, Size);
	if (Inner.IsError())
	{
		SetError();
	}
}

int64 FAssetArchive::Tell()
{
	return Inner.Tell();
}

int64 FAssetArchive::TotalSize()
{
	return Inner.TotalSize();
}

void FAssetArchive::Seek(int64 Position)
{
	if (Position < 0 || Position > TotalSize())
	{
		SetError();
		return;
	}

	Inner.Seek(Position);
}

void FAssetArchive::SerializeBool(bool& Value)
{
	// bool 메모리에 임의의 바이트를 직접 읽지 않고 0 또는 1인지 먼저 확인한다.
	uint8 Number = Value ? 1 : 0;
	*this << Number;
	if (Number > 1)
	{
		SetError();
	}

	if (IsLoading() && !IsError())
	{
		Value = Number != 0;
	}
}

FArchive& FAssetArchive::operator<<(FName& Value)
{
	FString Name = IsSaving() ? Value.ToString() : FString();
	*this << Name;

	if (IsLoading() && !IsError())
	{
		Value = FName(Name);
	}

	return *this;
}

FAssetArchive::EReferenceType FAssetArchive::GetReferenceType(UObject* Object) const
{
	if (!Object)
	{
		return EReferenceType::Null;
	}

	// 루트 애셋을 다시 참조하는 경우는 내부 객체 ID 0으로 기록한다.
	if (Object != Objects[0] && Object->IsA<UAsset>())
	{
		return EReferenceType::External;
	}

	return EReferenceType::Internal;
}

FArchive& FAssetArchive::operator<<(UObject*& Value)
{
	uint8 Type = static_cast<uint8>(GetReferenceType(Value));
	*this << Type;
	if (IsError())
	{
		return *this;
	}

	switch (static_cast<EReferenceType>(Type))
	{
	case EReferenceType::Null:
		Value = nullptr;
		break;
	case EReferenceType::Internal:
		SerializeInternalReference(Value);
		break;
	case EReferenceType::External:
		SerializeExternalReference(Value);
		break;
	default:
		SetError();
		break;
	}

	return *this;
}

int32 FAssetArchive::FindOrAddObject(UObject* Object)
{
	const int32 ExistingIndex = Objects.Find(Object);
	if (ExistingIndex >= 0)
	{
		return ExistingIndex;
	}

	if (!Object || Objects.Num() >= MaxObjects)
	{
		return -1;
	}

	// Outer 체인이 루트에 도달하는지 확인한다. 순환하거나 다른 소유자의 객체면 거부한다.
	UObject* Outer = Object->GetOuter();
	int32 Depth = 0;
	for (UObject* Parent = Outer; Parent != Objects[0]; Parent = Parent->GetOuter())
	{
		if (!Parent || ++Depth > MaxObjects)
		{
			return -1;
		}
	}

	// 로드 시 Outer부터 생성할 수 있도록 부모를 자식보다 먼저 등록한다.
	if (FindOrAddObject(Outer) < 0 || Objects.Num() >= MaxObjects)
	{
		return -1;
	}

	Objects.Add(Object);
	return Objects.Num() - 1;
}

void FAssetArchive::SerializeInternalReference(UObject*& Object)
{
	int32 Index = IsSaving() ? FindOrAddObject(Object) : -1;
	*this << Index;
	if (!Objects.IsValidIndex(Index))
	{
		SetError();
		return;
	}

	if (IsLoading())
	{
		Object = Objects[Index];
	}
}

void FAssetArchive::SerializeExternalReference(UObject*& Object)
{
	FString Path = IsSaving() ? Cast<UAsset>(Object)->GetPath() : FString();
	*this << Path;
	if (Path.empty())
	{
		SetError();
	}

	if (IsLoading() && !IsError())
	{
		FString Error;
		Object = UAssetManager::Get().LoadAssetReference(Path, Error);
		if (!Object)
		{
			SetError();
		}
	}
}
