# 히토리 리플렉션 TODO

목표는 언리얼식 네이티브 리플렉션 코어를 히토리의 기존 구조 위에 구현하는 것이다. 런타임에서 클래스·구조체·열거형의 정보를 조회하고, 등록된 함수를 이름으로 찾아 호출할 수 있으면 코어가 완성된 것으로 본다.

## 현재 상태

- `UCLASS()`와 지원 타입의 `UPROPERTY`는 HHT 코드 생성부터 런타임 등록까지 연결되어 있다. 디테일 패널, 직렬화, 시퀀서가 이 정보를 사용한다.
- `UClass`는 `UObject`를 직접 상속하고 `TArray<FProperty>`를 직접 보관한다. `USTRUCT()` 매크로는 정의되어 있으며, HHT의 스캔·파싱과 런타임 등록은 구현할 항목이다.
- `UENUM()`, `UFUNCTION()`, 숫자 `meta=(...)`는 HHT가 생성 코드를 출력한다. 생성 코드가 기대하는 런타임 타입과 API, 소비 코드의 연결이 남아 있다.

## 구현 순서와 완료 기준

### 1. `UStruct`와 `USTRUCT()` 연결

- [ ] `UStruct`에 공통 필드 목록과 이름 기반 조회를 두고, `UClass`와 `UScriptStruct`에서 공유한다. 기존 클래스의 상속 프로퍼티 조회를 확인한다.
- [ ] HHT가 `USTRUCT()`와 내부 `UPROPERTY`를 파싱해 `UScriptStruct` 등록 코드를 생성하도록 한다. 구조체 타입의 프로퍼티에서 하위 필드를 순회할 수 있게 연결한다.
- [ ] **완료 기준.** `float`와 `bool`이 있는 작은 구조체를 `UObject`의 `UPROPERTY` 멤버로 두고, 디테일 패널에서 두 필드를 각각 편집한다. 저장·재로드 후 두 값이 복원되고 기존 씬도 로드된다. 이후 구조체 리플렉션 적용 후보로 [`ALevelSequenceActor`의 `PlaybackSettings`](Source/Runtime/LevelSequence/LevelSequenceActor.h)를 검토한다.

### 2. `UENUM()`과 프로퍼티 타입 확장

- [ ] HHT의 `TEnumInfo` 생성 결과를 런타임 열거형 조회와 프로퍼티에 연결한다.
- [ ] `TArray` 프로퍼티에 원소 타입 정보와 원소 접근을 연결한다. 구조체·배열의 하위 값 접근, 타입에 맞는 값 복사 및 직렬화를 지원한다.
- [ ] **완료 기준.** 열거형 이름과 값으로 디테일 패널 드롭다운을 구성하고, 구조체 배열의 각 원소를 편집한 뒤 저장·복원할 수 있다.

### 3. `UFUNCTION()` 런타임 등록과 호출

- [ ] HHT가 생성하는 `RegisterFunctions()`를 실제 등록 경로에 연결한다. `UFunction`이 공통 필드 표현을 사용해 인자와 반환 타입을 설명하도록 한다.
- [ ] 함수 이름으로 조회하고, 공통 호출 경로에서 실제 C++ 함수에 인자를 전달하고 반환값을 받는다.
- [ ] **완료 기준.** 런타임에서 `Add`라는 이름으로 함수를 찾아 정수 `2`, `3`을 전달하고 반환값 `5`를 받는다.

### 4. 지정자와 진단 연결

- [ ] `VisibleAnywhere`의 읽기 전용 동작을 확인하고, `Transient`를 기존 JSON과 Structured Archive 직렬화 경로에서 일관되게 적용한다.
- [ ] `ClampMin`, `ClampMax`, `UIMin`, `UIMax`, `Delta` 같은 숫자 `meta`를 프로퍼티에 저장하고 디테일 패널에 연결한다.
- [ ] **완료 기준.** 읽기 전용 필드는 패널에서 수정할 수 없고, `Transient` 필드는 저장에서 빠진다. 숫자 편집 설정이 적용되며, 지원하지 않는 선언에는 파일과 줄 번호가 포함된 오류가 나온다.

## 범위

HHT의 C++20 구현과 `decltype` 기반 타입 확인은 유지할 수 있다. GC, 블루프린트, 네트워크 복제는 리플렉션 코어를 활용하는 별도 시스템으로 다룬다.

[히토리 코드 생성기](Source/Programs/HitoriHeaderTool/CodeGenerator.cpp) · [히토리 프로퍼티 표현](Source/Runtime/UObject/Property.h) · [Epic UStruct API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/CoreUObject/UStruct) · [Epic FProperty API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/CoreUObject/FProperty)
