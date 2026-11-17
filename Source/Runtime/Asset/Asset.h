#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"

#include "Asset.generated.h"

UCLASS()
class UAsset : public UObject
{
	GENERATED_BODY()
public:
	UAsset() = default;
	virtual ~UAsset() override;

	// 수동 Serialize 형식이 바뀌면 버전을 올린다. 첫 버전은 이전 형식을 거부한다.
	virtual uint32 GetAssetVersion() const { return 1; }
	virtual bool ValidateAsset(FString& OutError) const { return true; }
	virtual void PostLoadAsset() {}

	void SetPath(FString Path) { AssetPath = Path; }
	const FString& GetPath() const { return AssetPath; }

private:
	FString AssetPath;
};
