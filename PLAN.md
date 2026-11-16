# 파티클 시스템 기초 구현 계획

- 작업 브랜치: `feature/gyeongjae`
- 요구사항: [JUNGLE Compass WEEK12 — Dance of the Dust](https://jungle-lms.krafton.com/learning/1858)
- 이 문서는 구현 계획입니다. 아래 함수 이름과 시그니처는 제안이며 기존 엔진 규칙에 맞춰 조정할 수 있습니다.
- 기존 구현은 재사용하고, 각 단계의 완료 여부는 빌드와 실행 검증으로 판단합니다.

## 작업 범위

이번 완료 범위는 **렌더링 없는 모듈 기반 CPU 파티클 시스템**입니다.

포함할 항목:

- System / Emitter / LODLevel / Module 설정 계층
- 입자 저장소와 활성 인덱스 관리
- Spawn / Update 모듈 실행과 Payload 배치
- 기본 모듈과 생성·수명·이동·소멸 시뮬레이션
- ParticleSystemComponent의 초기화·Tick·재생·중단·정리
- 후속 렌더링 연결 인터페이스와 실행 검증

후속 작업으로 둘 항목:

- Particle System Editor, 파일 기반 파티클 에셋 저장·로드
- Sprite / Mesh 렌더링, Show Flag / Stat
- MiniDump / CauseCrash / Symbol Server
- 충돌 이벤트, Beam / Ribbon, 거리별 LOD 선택·전환
- 커브 에디터

에디터·렌더링·MiniDump·Symbol Server 등은 LMS의 필수 요구사항이므로 팀 최종 작업 목록에는 유지합니다. LOD 렌더링·충돌·Beam·Ribbon·커브 에디터는 선택 항목입니다. 에디터를 이번 개인 기초 범위에서 미루는 것은 제안이며, 확인된 팀 합의는 아닙니다.

## 1단계 — 실행 규칙과 데이터 계약 확정

상태: **정책 합의 완료**. 이번 단계에서는 문서만 갱신하며 코드는 변경하지 않습니다. 아래 정책의 실제 동작 구현과 검증은 2~9단계에서 수행합니다.

대상 파일:

- `Source/Runtime/Particles/BaseParticle.h`
- `Source/Runtime/Particles/ParticleEmitterInstance.h`

| 항목 | 구체적인 작업 |
|---|---|
| 수명 규칙 | `Lifetime`은 초, `RelativeTime`은 `경과 시간 / Lifetime`으로 통일 |
| 회전 규칙 | `Rotation`은 라디안, `RotationRate`는 라디안/초로 정의 |
| Base 속성 | `BaseSize`, `BaseColor` 등은 생성 시 기준값, `Size`, `Color`는 현재 결과값으로 정의 |
| 좌표계 | World Space 사용. 생성 시 컴포넌트 원점을 기준으로 Transform을 적용하여 월드 위치를 계산. 생성 이후에는 컴포넌트의 이동·회전·스케일 변경을 따라가지 않음 |
| 용량 정책 | 초기화 시 고정 용량 확보. 초과 생성 요청은 버리고 다음 프레임에 이월하지 않음. 소멸 슬롯 재사용. 추후 메모리 블록 확장을 추가할 수 있도록 할당 처리를 분리하되 자동 증설은 이번에 구현하지 않음 |
| 설정 공유 | System·Emitter·Module은 설정 데이터로 공유. 시간·입자·생성 누적값은 Instance에 보관 |
| 중단 정책 | 생성 중단 후 기존 입자는 계속 갱신. 즉시 제거는 별도 함수로 제공 |
| 설정 변경 | 실행 전 모듈 구성을 완료한 뒤 Instance 초기화. 실행 중 구성의 직접 변경은 지원하지 않음. 에디터에서 구성이 변경되면 해당 공유 설정을 사용하는 기존 Instance들을 정리하고, 캐시·메모리 배치를 재계산한 후 다시 생성하여 처음부터 재생 |

### 확정한 정책의 세부 의미

- `Lifetime`은 초 단위이며 `RelativeTime`은 정규화된 입자 나이입니다. `Rotation`은 라디안, `RotationRate`는 라디안/초입니다.
- Base 속성은 기준값, 대응하는 현재 속성은 모듈 평가 결과입니다. 프레임별 복원과 누적 여부는 각 모듈의 동작 계약에 맞춰 구현합니다.
- 생성된 입자는 월드 좌표를 보관합니다. 컴포넌트가 이동하면 새로 생성되는 입자의 출발 위치만 달라지고 기존 입자는 따라가지 않습니다.
- System·Emitter·Module은 공유 설정입니다. 입자 저장소, 경과 시간, 반복 횟수, 생성 누적값, 랜덤 상태, 모듈 Instance Payload는 각 실행 Instance가 독립적으로 소유합니다.
- 생성 중단 시 기존 입자는 계속 갱신하며 수명 종료 후 제거합니다. 전체 입자를 즉시 제거하는 별도 함수를 제공합니다.
- 모듈 추가·삭제 또는 타입 변경은 Payload 크기·오프셋·Stride를 바꿀 수 있습니다. 기존 저장소를 유지한 채 새 오프셋만 적용하지 않습니다.
- 에디터 구성 변경 시 기존 입자와 Instance 실행 메모리를 정리한 뒤, 변경된 구성으로 캐시·오프셋·Stride를 계산하고 Instance와 저장소를 다시 생성합니다. 기존 입자 상태를 이전하지 않습니다.
- 생성량·초기 색상 같은 단순 값 변경은 원칙적으로 재배치가 필요하지 않을 수 있으나, 이번 기초 구현에서는 명시적인 재초기화로 반영하는 정책으로 통일합니다.
- 공유 설정의 캐시를 재구성하기 전에 영향을 받는 Instance들을 모두 정리합니다. 이전 배치의 Instance와 새 캐시가 혼용되지 않도록 합니다. 에디터 변경 감지·자동 재생성 연결은 후속 에디터 작업입니다.

### 코드 반영 위치 — 이번 단계에서는 수정하지 않음

| 대상 | 후속 작업 |
|---|---|
| `BaseParticle.h` | 수명·회전 단위, 월드 위치, Base/현재 속성의 의미를 주석으로 명시. 현재 필드 구조 변경은 필수 아님 |
| `ParticleEmitterInstance.h/.cpp` | 생성 인자의 좌표 계약 명시, 생성 경로의 월드 변환과 이후 독립 갱신, 고정 용량·슬롯 재사용 유지, 할당 처리 분리, 생성 억제·전체 제거·초기화 구현 |
| `ParticleDataContainer.h/.cpp` | 할당·해제 책임 정리. 추후 블록 확장을 추가할 수 있는 저장소 경계 유지. 자동 확장은 미구현 |
| `ParticleSystem`, `ParticleEmitter`, `ParticleLODLevel`, `ParticleModule` | 공유 설정 계층과 실행용 캐시 구성. 재생 상태를 공유 설정 객체에 저장하지 않음 |
| `ParticleSystemComponent.h/.cpp` | Instance 생성·정리·재시작 경로 구현. 생성 중단 후 잔여 입자 Tick 유지 |
| 후속 Particle Editor | 설정 변경 후 영향받는 Instance 정리 → 캐시 재구성 → Instance 재생성 경로 연결 |

1단계 완료 기준: 정책과 각 필드·제어 함수의 의미가 문서에 확정되어 있습니다. 실제 동작이 이 계약을 만족하는지는 후속 구현 단계에서 검증합니다.

## 2단계 — 기존 입자 저장소 완성

대상 파일:

- `Source/Runtime/Particles/ParticleDataContainer.h/.cpp`
- `Source/Runtime/Particles/ParticleEmitterInstance.h/.cpp`

| 클래스·함수 | 작업 |
|---|---|
| `FParticleDataContainer::Allocate` | 크기 계산 오버플로, 재할당 시 기존 메모리 처리, 할당 실패 처리 확인 |
| `FParticleDataContainer::Free` | 반복 호출해도 안전하게 유지 |
| `FParticleDataContainer` | 복사로 인한 중복 소유 방지. 소멸 시 해제 책임을 한 곳으로 정리 |
| `FParticleEmitterInstance::InitializeParticleStorage` | 현재 선언만 있는 함수 구현. 할당·인덱스 초기화를 이 함수로 모음 |
| `Initialize` | 설정 연결과 저장소 초기화 역할 분리 |
| `SpawnParticles` | 기존 슬롯 재사용·용량 제한 로직 유지. 모듈 초기화 지점 확보 |
| `KillParticle`, `KillParticles` | 마지막 활성 인덱스와 교환하는 기존 방식 검증 |
| `KillAllParticles` 추가 | 저장소를 유지하면서 활성 입자 전체 제거 |
| `Reset` 추가 | 활성 입자, 누적 시간, 생성 카운터·누적값 초기화 |
| `GetParticle`, `GetParticleSlot` | 잘못된 인덱스와 초기화 전 접근 검증 |

Payload가 추가되면 슬롯 시작 주소뿐 아니라 Stride와 각 Payload 오프셋도 정렬 조건을 만족해야 합니다. 현재 컨테이너의 2바이트 정렬 검사만으로는 이 요구를 모두 보장하지 못합니다.

완료 기준: 생성 → 중간 입자 제거 → 재생성 시 슬롯이 정상 재사용되고, 반복 초기화·해제에도 문제가 없습니다.

## 3단계 — System / Emitter / LODLevel 설정 계층 연결

대상 파일:

- `Source/Runtime/Particles/ParticleSystem.h/.cpp`
- `Source/Runtime/Particles/ParticleEmitter.h/.cpp`
- `Source/Runtime/Particles/ParticleLODLevel.h/.cpp`

| 클래스 | 추가할 데이터와 함수 |
|---|---|
| `UParticleSystem` | `Emitters`, `AddEmitter`, `GetEmitters`, `Validate`, `Build` |
| `UParticleEmitter` | `LODLevels`, `AddLODLevel`, `GetLODLevel`, `Validate`, `CacheEmitterModuleInfo` |
| `UParticleLODLevel` | `Level`, `bEnabled`, `RequiredModule`, `SpawnModule`, `TypeDataModule`, `Modules` |
| `UParticleLODLevel` | `AddModule`, `RemoveModule`, `Validate`, `BuildModuleLists` |
| `UParticleLODLevel` | 파생 실행 목록 `SpawnModules`, `UpdateModules` 구성 |

구현 규칙:

- 이번에는 LOD 0만 실행합니다.
- `RequiredModule`과 `SpawnModule`은 Emitter마다 하나씩 둡니다.
- 같은 모듈이 전용 필드와 일반 목록에 중복되어 두 번 실행되지 않게 합니다.
- 설정 객체의 Outer 관계를 구성하되, Outer 지정만으로 자동 해제가 보장된다고 가정하지 않습니다.
- UObject 클래스에 `GENERATED_BODY()`와 `.generated.h`를 연결하여 `NewObject<T>()`가 올바른 클래스 정보를 사용하도록 합니다.
- 실행용 캐시와 포인터는 저장할 설정 데이터와 구분합니다.

완료 기준: 코드에서 System 하나에 Emitter 여러 개와 모듈을 구성하고, 누락된 필수 설정을 `Validate`로 탐지할 수 있습니다.

## 4단계 — 모듈 인터페이스·캐싱·Payload 배치

상태: **구현 및 독립 검증 완료, 사용자 검수 대기**. HeaderTool 처리와 MSVC Debug/Release + AddressSanitizer 검증을 수행했습니다. 전체 엔진 빌드·월드 실행은 이번 독립 검증에 포함하지 않습니다.

대상 파일:

- `Source/Runtime/Particles/ParticleModule.h/.cpp`
- `Source/Runtime/Particles/ParticleEmitter.h/.cpp`
- `Source/Runtime/Particles/ParticleEmitterInstance.h/.cpp`
- `Source/Runtime/Particles/ParticleHelper.h`

| 클래스·함수 | 작업 |
|---|---|
| `UParticleModule` | `bEnabled`, Spawn/Update 참여 여부 정의 |
| `UParticleModule::Spawn` | 새 입자 하나의 초기 속성을 설정하는 가상 함수 |
| `UParticleModule::Update` | 활성 입자들을 갱신하는 가상 함수 |
| `RequiredBytes` | 입자별 Payload 크기 반환 |
| `RequiredBytesPerInstance` | EmitterInstance별 Payload 크기 반환 |
| Payload 정렬 조회 함수 | 입자·인스턴스 Payload에 필요한 정렬값 반환 |
| `InitializeInstance` | 모듈의 인스턴스 상태 초기화 |
| `FParticleModuleBinding` 추가 | 모듈 포인터, 입자 Payload 오프셋, 인스턴스 Payload 오프셋 보관 |
| `CacheEmitterModuleInfo` | 활성 모듈 수집, 오프셋 배치, 최종 `ParticleSize`·`ParticleStride`·`InstancePayloadSize` 계산 |
| `FParticleEmitterInstance::Initialize` | 캐시된 크기로 저장소와 인스턴스 Payload 할당 |
| Payload 접근 함수 | 모듈이 원시 저장소를 직접 다루지 않고 필요한 영역을 얻도록 제공 |

`Spawn`에는 생성 시점 보정을 위한 시간을 전달합니다. `Update`에는 `DeltaTime`과 해당 모듈의 Payload 오프셋을 전달하는 형태로 설계합니다.

현재 `ParticleHelper.h`의 매크로는 아직 정의되지 않은 `Context`, 플래그와 Instance의 private 멤버에 의존합니다. 위 인터페이스에 맞춰 정리해야 합니다. 처음에는 명시적인 반복문과 접근 함수를 사용하고, 반복 패턴이 확정되면 매크로를 연결합니다.

완료 기준: 검증용 모듈 두 개가 서로 다른 Payload 영역을 사용하고, 같은 설정을 공유하는 두 Instance의 Payload도 독립적으로 유지됩니다.

### 4단계 구현 계약과 검증

- `ParticleHelper.h`에 Binding·Layout과 모듈 초기화/Spawn/Update Context를 정의했습니다. 사용되지 않던 미완성 원시 접근 매크로는 명시적 반복문·접근 함수로 대체했습니다.
- `UParticleEmitter::Build`는 LOD별 모듈 배치를 캐싱합니다. Instance는 해당 배치를 복사하며 설정 객체는 계속 공유합니다. 실행 중 설정 변경은 지원하지 않습니다.
- Payload 크기 0은 추가 데이터가 없음을 의미합니다. 음수 크기, 오버플로, 잘못된 정렬은 거부합니다. 정렬은 2의 거듭제곱이며 현재 `malloc`이 보장하는 `alignof(std::max_align_t)` 이하로 제한합니다.
- Payload는 별도 소멸 처리 없이 재사용 가능한 단순 데이터만 보관합니다. 모듈의 `InitializeInstance`, `Spawn`, `Update` 콜백은 `const noexcept`이며 공유 모듈 자체에 재생 상태를 쓰지 않습니다.
- Instance 생성과 Reset에서 인스턴스 Payload를 초기화합니다. 입자 생성·슬롯 재사용 시 입자 Payload를 초기화하고 Spawn 모듈을 순서대로 실행합니다.
- `UpdateModules`는 모듈 Update 호출만 담당합니다. 수명 갱신·이동 적분·생성 스케줄을 포함하는 전체 Tick은 6단계에서 연결합니다.
- 검증 코드는 `Temp/ParticlePayloadVerification/`에 있습니다. 객체 생성·등록·해제 로직은 현재 엔진 소스에서 추출한 검증용 파일을 사용하며, 범위 밖인 직렬화 세 함수만 대체했습니다.
- Debug/Release + AddressSanitizer에서 각각 68,391개 조건 검사에 통과했습니다. Payload 분리·정렬·순서·Instance 독립성·Reset·잘못된 배치 거부와 저장소/설정 계층 회귀 검사를 포함합니다. 실제 메모리 부족을 강제로 발생시키는 검증은 포함하지 않습니다.

## 5단계 — 기본 모듈 구현

상태: **상수값 기본 모듈 구현 및 독립 검증 완료, 사용자 검수 대기**. 기본 생성 속성만 연결했으며 전체 Tick과 생성 스케줄은 6단계에서 구현합니다.

신규 파일 위치: `Source/Runtime/Particles/Modules/`. 클래스별 `.h/.cpp`로 추가합니다.

| 신규 클래스 | 주요 설정 | 구현할 함수·동작 |
|---|---|---|
| `UParticleModuleRequired` | 최대 입자 수, Emitter 지속 시간·반복 횟수 | `Validate`. Emitter 실행에 필요한 공통 설정 제공 |
| `UParticleModuleSpawn` | 초당 생성량 | 생성량 조회 함수. 누적 상태는 Instance가 보관 |
| `UParticleModuleLifetime` | 수명 | `Spawn`: `Lifetime` 설정, 유효 범위 검사 |
| `UParticleModuleLocation` | 초기 위치 오프셋 | `Spawn`: 초기 위치 설정 |
| `UParticleModuleVelocity` | 초기 속도 | `Spawn`: `BaseVelocity`, `Velocity` 설정 |
| `UParticleModuleColor` | 초기 색상 | `Spawn`: `BaseColor`, `Color` 설정 |
| `UParticleModuleSize` | 초기 크기 | `Spawn`: `BaseSize`, `Size` 설정 |
| `UParticleModuleTypeDataBase` | Emitter 타입 인터페이스 | Sprite/Mesh 종류 조회 |
| `UParticleModuleTypeDataMesh` | 메시 설정 | Mesh 타입과 메시 참조 제공 |

Sprite는 기본 타입으로 처리하고, Mesh는 TypeData로 구분합니다. 타입 구분과 설정까지만 포함하며 Mesh 렌더링은 구현하지 않습니다.

첫 버전은 상수값으로 완성합니다. 이후 확장 작업은 별도로 분리합니다.

- Lifetime·Location·Velocity의 최소/최대 랜덤값
- `UParticleModuleColorOverLife::Update`
- `UParticleModuleSizeOverLife::Update`
- Burst
- 가속도 모듈

완료 기준: 각 모듈의 값을 바꾸면 생성된 입자의 해당 속성만 예상대로 바뀝니다.

### 5단계 구현 계약과 검증

- 기존 Required·Spawn·TypeDataBase는 `ParticleModule.h/.cpp`에 유지했습니다. Lifetime·Location·Velocity·Color·Size·Mesh TypeData는 `Particles/Modules/`에 클래스별 헤더·구현을 추가했습니다.
- Required는 최대 입자 수(1~65535), 양수의 유한한 지속 시간, 반복 횟수(0은 무한)를 보관합니다. 필수 Required 모듈을 비활성화한 설정은 거부합니다. 지속 시간과 반복 처리 자체는 6단계에 남아 있습니다.
- Spawn은 0 이상의 유한한 초당 생성량을 보관하고 `GetSpawnRate`로 제공합니다. 비활성화하면 0을 반환하며 누적 생성 상태는 보관하지 않습니다.
- Lifetime은 양수의 유한한 수명을 설정합니다. Location·Velocity는 기본 월드 생성 위치·속도에 초기값을 더하고, Size·Color는 Base/현재 값을 함께 설정합니다. 크기 성분은 유한한 0 이상입니다.
- 일반 초기화 모듈은 Spawn에만 참여하며 추가 Payload를 요구하지 않습니다. 여러 Location·Velocity 모듈은 순서대로 더해지고, 여러 Lifetime·Size·Color 모듈은 마지막 적용값을 사용합니다.
- `Initialize`의 4인자 경로는 Required의 용량을 사용합니다. 용량을 명시하는 기존 경로도 유지하며 이 경우 인자가 우선합니다.
- `SpawnParticles`의 기존 위치·속도 인자는 월드 값입니다. 마지막 선택적 생성 행렬을 전달하면 Location·Velocity 모듈의 초기값에만 회전·스케일을 적용합니다. 위치 인자에 포함된 생성 원점의 이동은 중복 적용하지 않습니다. 행렬을 보관하지 않으므로 이후 변환 변경은 기존 입자에 영향을 주지 않습니다. 컴포넌트에서 행렬과 생성 원점을 전달하는 연결은 7단계입니다.
- Mesh TypeData는 외부 `UStaticMesh` 참조와 Mesh 타입을 제공하며 참조가 없으면 검증에 실패합니다. 비활성화하거나 제거하면 LOD 타입 조회는 기본 Sprite를 반환합니다. 실제 메시 에셋 로드와 렌더링은 이번 단계에서 구현하지 않습니다.
- 모듈 초기 위치를 적용한 후 `OldLocation`을 저장하고, 최종 속도·수명으로 생성 시점의 이동과 RelativeTime을 보정합니다.
- 새 소스가 포함되도록 Premake 프로젝트를 재생성했습니다. HeaderTool은 파티클 헤더 10개를 오류 없이 처리했습니다.
- `Temp/ParticleModuleVerification/`의 실제 모듈 실행 검증에서 Debug/Release + AddressSanitizer 각각 1,400개 조건 검사를 통과했습니다. 25회 생성·해제를 포함하며 직렬화는 대체 함수를 사용합니다. 전체 엔진 링크·PSC 월드 실행·실제 메시 에셋 로드는 검증 범위에 포함하지 않습니다.

## 6단계 — Emitter의 프레임 시뮬레이션 구현

상태: **구현 및 독립 검증 완료, 사용자 검수 대기**. PSC의 월드 Tick 연결은 7단계에서 진행합니다.

대상 파일: `Source/Runtime/Particles/ParticleEmitterInstance.h/.cpp`

| 함수 | 구체적인 역할 |
|---|---|
| `Tick(float DeltaTime)` | 한 프레임의 전체 시뮬레이션 진입점 |
| `UpdateEmitterTime` | Emitter 경과 시간·지속 시간·반복 횟수 갱신 |
| `UpdateParticleTimes` | 기존 입자의 `RelativeTime += DeltaTime / Lifetime` |
| `ResetParticleParameters` | 수명별 Size·Color 등이 이전 프레임 결과에 누적 적용되지 않도록 기준값 복원 |
| `UpdateModules` | 캐시된 Update 모듈 순서대로 실행 |
| `IntegrateParticles` | `OldLocation` 저장, 위치·회전 적분 |
| `Spawn` | 생성량 누적과 실제 생성 요청 |
| `PreSpawn` | 새 슬롯 전체 초기화와 기본 상태 설정 |
| `SpawnParticles` | Spawn 모듈 순회와 활성 목록 편입 |
| `PostSpawn` | 프레임 중간에 태어난 입자의 나이·이동 보정 |
| `HasCompleted` | 생성 종료 및 활성 입자 0개인지 반환 |
| `SetSpawningEnabled` | 기존 입자를 유지하면서 신규 생성만 제어 |

추가 실행 상태: `EmitterTime`, 반복 카운터, 소수 생성 누적값, 생성 허용 여부 등.

Tick 순서:

1. 기존 입자의 수명을 갱신하고 만료 입자를 제거합니다.
2. 기존 입자의 기준 속성을 복원하고 Update 모듈을 실행합니다.
3. 기존 입자의 위치·회전을 적분합니다.
4. 해당 프레임의 생성 수와 생성 시점을 계산합니다.
5. 새 입자에 Spawn 모듈을 적용하고 프레임 끝까지의 시간만 보정합니다.

신규 입자가 전체 `DeltaTime`으로 다시 갱신되어 생성 첫 프레임에 두 번 이동하지 않도록 분리합니다.

완료 기준: 같은 총시간을 서로 다른 프레임 간격으로 실행했을 때 생성량·나이·위치가 정의한 허용 오차 안에서 일치합니다. 용량 포화 등 정책상 프레임 분할에 영향을 받는 조건은 별도로 검증합니다.

### 6단계 구현 계약과 검증

- `Tick`은 기존 입자의 시간 갱신·만료 제거 → Base 속성 복원 → Update 모듈 → 만료 제거 → 이동·회전 적분 → 신규 생성·생성 시점 보정 → 만료 제거 순서로 실행합니다. 신규 입자는 같은 프레임의 기존 입자 Update·적분을 다시 받지 않습니다.
- 생성은 상수 SpawnRate를 사용하며 첫 입자는 생성 간격이 지난 시점에 태어납니다. 프레임 끝 시점의 생성도 포함합니다. 소수 생성량은 다음 프레임으로 누적하지만 용량 초과 정수 요청은 버립니다.
- Emitter 시간·생성 누적값·슬롯별 입자 경과 시간은 double로 보관합니다. 외부 `SpawnParticles` API는 유지하며 스케줄러의 생성 시각은 내부 double 경로로 전달합니다. `RelativeTime`은 실제 경과 시간과 수명으로 매 프레임 다시 계산합니다.
- Required의 지속 시간·반복 횟수와 SpawnRate는 초기화 시 복사합니다. 0회 반복 설정은 무한 반복입니다. 여러 반복을 가로지르는 큰 DeltaTime도 반복문 없이 계산하고 유한 반복의 마지막 생성 구간만 사용합니다. 무한 반복 카운터가 uint64 범위를 넘으면 최댓값으로 고정합니다.
- `EmitterTime`은 현재 반복의 경과 시간입니다. 유한 반복이 끝나면 0으로 고정하고 완료 반복 수를 반환합니다. `SecondsSinceCreation`은 잔여 입자 갱신 중에도 계속 증가합니다. 부동소수점 경계에는 작은 허용 오차를 적용합니다.
- `SetSpawningEnabled(false)`는 새 입자만 막고 소수 생성 누적값을 비웁니다. Tick 시간과 기존 입자는 계속 갱신합니다. 재허용 시 중단 기간의 입자를 몰아서 생성하지 않습니다. 이미 유한 반복이 끝났다면 Reset이 필요합니다.
- `HasCompleted`는 생성 종료 또는 생성 중단 상태에서 활성 입자가 0개일 때 true입니다. 무한 반복·생성량 0인 설정은 시간상 종료가 없으므로 중단하기 전에는 완료로 판정하지 않습니다.
- `KillAllParticles`는 입자만 제거하고 Emitter의 생성·시간 상태는 유지합니다. `Reset`은 같은 설정의 입자·시간·반복·소수 누적값·카운터·모듈 상태를 초기화하고 생성을 다시 허용합니다. 설정 변경 반영은 Reset이 아니라 Instance 재초기화 경로를 사용합니다.
- 프레임마다 Velocity·Size·Color·RotationRate를 Base 값으로 복원합니다. Rotation과 Location은 누적 적분하고, 생성 직후에는 Spawn에서 설정한 RotationRate와 실제 생성 후 경과 시간으로 보정합니다.
- 입자 저장소와 함께 슬롯별 double 경과 시간 배열을 소유합니다. 저장소 재초기화 실패 시 이전 저장소를 유지하는 구조도 보존합니다.
- 프레임 분할 동등성은 용량에 여유가 있고 현재의 상수 생성·수명·속도 조건에서 검증합니다. 한 프레임의 생성 요청이 여유 용량을 넘으면 배치 단위로 초과 요청을 버리므로 프레임 분할에 따라 결과가 달라질 수 있습니다. 프레임 중간의 슬롯 점유를 재현하는 이벤트 기반 시뮬레이션이나 서브스텝은 이번 범위가 아닙니다.
- 검증 코드는 `Temp/ParticleTickVerification/`에 있습니다. 유한·무한 반복, 생성 중단/재개, 종료 후 잔여 입자, 용량 초과 요청 폐기, Base 속성 복원, 회전, 10,000회 미세 프레임, 소멸 구간의 프레임 분할 비교, 잘못된 DeltaTime과 큰 입력 경계를 확인합니다. UObject 직렬화는 검증용 대체 함수를 사용하며 전체 엔진·PSC 월드 실행은 포함하지 않습니다.
- HeaderTool 처리와 MSVC Debug/Release + AddressSanitizer에서 각각 133개 조건 검사 및 10,000회 미세 프레임 실행을 통과했습니다.
- `SpawnParticles`는 기존의 명시적 수동 생성 API로 유지합니다. 생성량·반복·생성 중단을 적용하는 자동 생성 경로는 `Tick`의 스케줄러입니다. PSC의 재생 제어는 7단계에서 이 경로에 연결합니다.

## 7단계 — ParticleSystemComponent 생명주기 연결

대상 파일: `Source/Runtime/Components/ParticleSystemComponent.h/.cpp`

| 함수 | 구체적인 역할 |
|---|---|
| 생성자 | Tick 사용 설정, 포인터·상태 초기화 |
| `SetTemplate(UParticleSystem*)` | 기존 실행 상태 정리 후 새 설정 연결 |
| `InitializeSystem` | 설정 검증·캐싱 후 EmitterInstance 생성 |
| `ReleaseInstances` | 실행 인스턴스와 관련 메모리 정리 |
| `InitializeComponent` | 컴포넌트 초기화 경로 연결 |
| `Activate(bool bReset)` | 최초 시작·재개·재시작 처리 |
| `Deactivate` | 신규 생성 중단, 잔여 입자 갱신 유지 |
| `ResetSystem` | 시간·입자·생성 누적값 초기화 |
| `TickComponent(float DeltaTime)` | 각 EmitterInstance 갱신, 전체 종료 판정 |
| `OnUnregister`·소멸자 | 중복 호출에도 안전하게 자원 정리 |
| `GetTotalActiveParticleCount` | 전체 활성 입자 수 조회 |
| `GetEmitterInstance` | 검증 및 후속 렌더링 연결용 const 접근 제공 |

현재 부모 클래스의 `Deactivate()`는 Tick을 즉시 끕니다. PSC에서는 생성 중단 후 잔여 입자가 모두 소멸할 때까지 Tick을 유지하는 처리가 필요합니다.

추가 작업:

- `GENERATED_BODY()`와 generated 헤더 연결
- 실행 인스턴스의 복제·직렬화 방지 및 재구성 정책
- 렌더링 구현 전 불필요한 SceneProxy를 만들지 않도록 `ShouldCreateRenderState()` 정책 지정
- 같은 Template을 공유하는 PSC 간 실행 상태 분리

완료 기준: 월드에 등록된 PSC가 자동 재생되고, 생성 중단·재시작·등록 해제 시 정상 동작합니다.

## 8단계 — 렌더링 연결 경계와 실행 예제 마련

대상: PSC, 기존 Dynamic Emitter 데이터 파일, 신규 검증용 예제 파일.

| 항목 | 작업 |
|---|---|
| 입자 조회 | 활성 개수와 const 입자 접근 제공. 활성 인덱스와 실제 슬롯 구분 |
| 타입 정보 | Sprite/Mesh 타입 및 머티리얼·메시 설정 접근 경로 정의 |
| 좌표 정보 | World Space 입자라는 계약 명시 |
| 데이터 수명 | 조회 결과를 다음 Tick·Reset·해제 이후 보관하면 안 된다는 규칙 명시 |
| ReplayData | 후속 렌더링에서 복사 스냅샷이 필요할 때 생성할 위치·소유권 정의 |
| 실행 예제 | 코드에서 System·Emitter·기본 모듈을 구성하고 PSC에 할당하는 예제 제공 |

기존 `FDynamicEmitterDataBase`, `FDynamicSpriteEmitterData`, `FDynamicMeshEmitterData`는 렌더링 담당 영역과 맞물리므로 이번에는 임의로 완성하지 않고 인터페이스를 맞춥니다.

기존 `AParticleActor`는 SubUV 용도로 사용 중이므로 바로 교체하지 않습니다. 별도 예제로 새 PSC를 검증하고 통합 시 Actor 변경 여부를 결정합니다.

완료 기준: 후속 렌더러가 시뮬레이션 내부 구현을 변경하지 않고 타입과 활성 입자 상태를 조회할 수 있습니다.

## 9단계 — 검증 코드·빌드·통합 확인

신규 검증 코드 위치 제안:

- `Source/Runtime/Particles/Test/ParticleSystemTest.h`
- `Source/Runtime/Particles/Test/ParticleSystemTest.cpp`

별도 테스트 프레임워크를 먼저 도입하지 않고 명시적으로 호출하는 검증 진입점을 마련합니다.

| 검증 함수 예시 | 확인할 내용 |
|---|---|
| `TestParticleStorage` | 용량 제한, 중간 제거, 슬롯 재사용, 반복 초기화 |
| `TestModuleSpawn` | 기본 모듈 각각의 초기값 적용 |
| `TestModulePayload` | 오프셋·정렬·모듈 간 데이터 분리 |
| `TestParticleLifetime` | 수명 경계와 만료 입자 제거 |
| `TestSpawnRate` | 소수 누적, 프레임 분할, 용량 초과 정책 |
| `TestParticleMotion` | 일정 속도 이동과 생성 시점 보정 |
| `TestEmitterCompletion` | 지속 시간·반복 종료 후 잔여 입자 처리 |
| `TestSharedTemplate` | 같은 설정을 공유하는 두 Instance의 독립성 |
| `TestComponentLifecycle` | Activate·Deactivate·Reset·재등록·해제 |

빌드·실행 확인:

1. 새 소스가 Premake의 Runtime 소스 패턴에 포함되는지 확인합니다.
2. 새 파일이 생성 프로젝트에 들어가도록 프로젝트를 재생성합니다.
3. 기존 HeaderTool로 generated 헤더를 생성합니다.
4. 네이티브 MSBuild로 Debug/Release를 빌드하고 검증 코드를 실행합니다.
5. 월드에 연결한 PSC의 Tick과 종료 동작을 실제 실행으로 확인합니다.

## 최종 완료 시나리오

- 초당 10개를 생성하고 각 입자가 지정 속도로 2초 동안 이동한 뒤 소멸합니다.
- 생성 중단 후에도 기존 입자는 남은 수명 동안 갱신됩니다.
- 초기화 후 재생하면 시간·생성 누적값·활성 입자가 초기 상태에서 시작합니다.
- 같은 `UParticleSystem`을 사용하는 두 PSC의 실행 상태가 독립적입니다.
- 여러 Emitter와 저장소 용량 제한이 정상 동작합니다.
- Spawn/Update 모듈과 Payload 접근이 실제 실행됩니다.

## 구현 순서

저장소 검증 → 설정 계층 → 모듈 인터페이스·메모리 배치 → 기본 모듈 → 시뮬레이션 → PSC 연결 → 렌더링 연결 경계 → 통합 검증.

특히 4단계의 인터페이스와 메모리 배치를 확정한 뒤 이후 기능을 구현합니다. 각자 기초 전체를 구현하는 동안에는 이 문서의 데이터 계약과 최종 완료 시나리오를 공통 비교 기준으로 사용합니다.

## 추가 단계 — UParticleSystem 에셋화 (기본 구현 범위 밖)

이 단계는 1~9단계의 기본 구현 완료 조건에 포함하지 않습니다. 기본 구현 이후 별도 작업으로 진행하며, 현재 구현 순서와 완료 기준은 변경하지 않습니다.

### 저장 단위와 데이터 구분

- 외부에 노출하는 에셋은 `UParticleSystem` 하나입니다. 별도 포장 클래스는 만들지 않고, Emitter·LODLevel·Module 설정을 같은 파티클 파일 내부에 저장합니다.
- 내부 설정 객체의 클래스 식별자, 구성 순서, 활성 여부와 모듈 설정값을 저장합니다. 파일 형식 버전도 포함합니다.
- 머티리얼·메시·텍스처 등 외부 에셋은 내용을 중복 저장하지 않고 경로 등 지속 가능한 식별자로 참조합니다.
- Required·Spawn·TypeData 포인터는 복원한 모듈 목록에서 재연결합니다. 동일 모듈을 별도 객체로 중복 복원하지 않습니다.
- Spawn/Update 실행 목록, Payload 오프셋, ParticleStride 등 파생 캐시는 저장하지 않고 로드 후 재구성합니다.
- 활성 입자, 경과 시간, 생성 누적값, 실행 Payload 등 Instance의 실행 상태는 저장하지 않습니다.

### 작업 목록

| 대상 | 작업 |
|---|---|
| `ParticleSystem.h/.cpp` 및 전용 직렬화 코드 | 파일 형식·버전 정의, 내부 설정 계층의 저장·로드 진입점 구현 |
| `ParticleEmitter`, `ParticleLODLevel` | 새 효과 생성과 파일 복원 경로 분리. 기본 Required·Spawn 자동 생성으로 인한 중복을 피하도록 빈 구성 복원 경로 마련 |
| `ParticleModule` 및 구체적인 모듈 | 실제 클래스 식별자와 설정값 저장·복원. 허용 클래스·필드·값 범위 검증 |
| 로드 처리 | 임시 설정 계층에 복원·검증한 후 채택. 실패 시 임시 객체 정리 및 기존 유효 에셋 유지 |
| 로드 후 처리 | 소유 관계·특수 모듈 참조 복원 후 `Validate`와 `Build`를 수행하여 실행 캐시 재구성 |
| `AssetManager` | 파티클 전용 로드·등록·캐싱 경로 또는 공통 에셋 관리 확장. 현재 `URenderAsset*` 중심 API에 그대로 등록할 수 없다는 점 반영 |
| `ParticleSystemComponent`와 씬 직렬화 | Template을 지속 가능한 에셋 참조로 저장하고, 씬 로드 시 공유 `UParticleSystem`으로 복원 |
| 에셋 수명 관리 | 참조하는 PSC·Instance보다 System이 오래 유지되도록 관리. 재로드 시 기존 Instance 정리 → 설정 교체 → Instance 재생성 순서 보장 |

현재 일반 UObject 참조 직렬화와 `UPROPERTY` 등록만으로 내부 설정 객체의 생성·복원이 자동 구현된다고 가정하지 않습니다. 파티클 전용 저장 스키마를 우선 검토하고, 기존 엔진 직렬화·에셋 관리 방식과 연결합니다.

### 검증 및 완료 기준

- 여러 Emitter·LOD·구체적인 Module을 포함한 설정을 저장하고 다시 읽었을 때 클래스·순서·설정값이 유지됩니다.
- Required·Spawn의 중복 생성 없이 소유 관계와 참조가 복원됩니다.
- 로드 후 캐시를 재구성하여 기본 시뮬레이션을 실행할 수 있습니다.
- 같은 경로를 사용하는 여러 PSC가 같은 설정 에셋을 공유하고 실행 상태는 독립적으로 유지됩니다.
- 씬 저장·로드 후 PSC의 Template 참조가 복원됩니다.
- 잘못된 버전·클래스·설정값·외부 참조와 손상된 파일을 처리하는 정책을 검증하며, 실패 시 기존 설정을 훼손하거나 임시 객체를 누수하지 않습니다.
- 저장 파일에 활성 입자나 실행 Payload가 포함되지 않음을 확인합니다.
