#pragma once

#include "Serialization/Archive.h"
#include "Containers/Array.h"

// 메모리 아카이브에 애셋의 내부 객체 ID와 외부 경로 참조 처리를 추가한다.
class FAssetArchive : public FArchive
{
public:
	static constexpr int32 MaxObjects = 4096;
	static constexpr int32 MaxFileSize = 64 * 1024 * 1024;

	FAssetArchive(FArchive& InInner, TArray<UObject*>& InObjects);
	using FArchive::operator<<;

	void Serialize(void* Data, int64 Size) override;
	void SerializeBool(bool& Value) override;
	int64 Tell() override;
	int64 TotalSize() override;
	void Seek(int64 Position) override;

	FArchive& operator<<(FName& Value) override;
	FArchive& operator<<(UObject*& Value) override;

private:
	// 값은 파일에 기록되므로 기존 순서를 유지해야 한다.
	enum class EReferenceType : uint8 { Null = 0, Internal = 1, External = 2 };

	EReferenceType GetReferenceType(UObject* Object) const;
	int32 FindOrAddObject(UObject* Object);
	void SerializeInternalReference(UObject*& Object);
	void SerializeExternalReference(UObject*& Object);

	FArchive& Inner;
	TArray<UObject*>& Objects;
};
