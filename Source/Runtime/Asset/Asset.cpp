#include "EnginePCH.h"
#include "Asset/Asset.h"

UAsset::~UAsset()
{
	// Outer 아래의 객체만 소유한다. 외부 애셋 참조는 함께 삭제하지 않는다.
	TArray<UObject*> OwnedObjects;
	OwnedObjects.Add(this);
	for (int32 Index = 0; Index < OwnedObjects.Num(); ++Index)
	{
		for (UObject* Object : GUObjectArray)
		{
			if (Object->GetOuter() == OwnedObjects[Index] && OwnedObjects.Find(Object) < 0)
				OwnedObjects.Add(Object);
		}
	}

	// 다른 소멸자나 엔진 종료 루프가 이미 지운 객체는 건너뛴다.
	for (int32 Index = 1; Index < OwnedObjects.Num(); ++Index)
	{
		if (GUObjectArray.Find(OwnedObjects[Index]) >= 0)
			delete OwnedObjects[Index];
	}
}
