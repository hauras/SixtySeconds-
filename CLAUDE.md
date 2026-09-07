# SixtySeconds — UE5 Project

## 게임 컨셉
로봇 침공 생존 게임 (60 Seconds! 스타일)

### 두 페이즈 구조
| 페이즈 | 형태 | 설명 |
|--------|------|------|
| 60초 스크램블 | 3D 액션 | 제한 시간 60초 내 맵에서 아이템·동료 수집 |
| 은신처 생존 | 2D 턴제 | 수집 자원으로 날짜별 이벤트 대응, 생존 결정 |

### 핵심 메카닉
- 동행자 2명 (그 중 한 명이 로봇일 수 있음 — 정체 공개 이벤트)
- 상태이상 시스템: GAS(Gameplay Ability System)로 구현
- 자원 관리, 로봇 침공 이벤트, 생존 결정

---

## 기술 스택
- **Engine**: Unreal Engine 5 (C++ + Blueprint 혼용)
- **Ability System**: Gameplay Ability System (GAS)
- **Target Platform**: PC (Windows)
- **Build System**: UnrealBuildTool (UBT)

---

## 아키텍처 개요

### 모듈 구조
```
Source/
├── SixtySeconds/          # 메인 게임 모듈
│   ├── Core/              # GameMode, GameState, GameInstance
│   ├── Phase/             # 페이즈 전환 (스크램블 ↔ 은신처)
│   ├── Companion/         # 동행자 시스템 (로봇 정체 포함)
│   ├── Shelter/           # 은신처 상태, 이벤트
│   ├── Item/              # 아이템 데이터, 인벤토리
│   └── UI/                # HUD, 위젯
└── SixtySecondsGAS/       # GAS 전용 모듈
    ├── Attributes/        # AttributeSet
    ├── Abilities/         # GameplayAbility
    └── Effects/           # GameplayEffect
```

### 핵심 시스템 클래스
| 클래스 | 역할 |
|--------|------|
| `ASSGameMode` | 페이즈 전환, 전체 게임 흐름 제어 |
| `USSPhaseManager` | 스크램블/은신처 전환 로직 |
| `USSCompanionSubsystem` | 동행자 관리, 로봇 여부 추적 |
| `USSAttributeSet` | HP, 허기, 방사능, 정신력 등 |
| `USSShelterManager` | 은신처 상태, 날짜, 이벤트 |

---

## GAS 설계 원칙
- `AbilitySystemComponent`는 **PlayerState**에 부착 (멀티플레이 확장 대비)
- **GameplayTag 네임스페이스**:
  - `Status.*` — 상태이상 (Status.Poisoned, Status.Irradiated 등)
  - `Ability.*` — 능력
  - `Item.*` — 아이템 분류
  - `Event.*` — 은신처 이벤트
  - `Phase.*` — 게임 페이즈 (Phase.Scramble, Phase.Shelter)
- 상태이상은 `GameplayEffect` + Duration/Stack으로 관리
- Attribute 변화는 항상 GE를 통해 적용 (직접 Set 금지)

---

## 코딩 컨벤션
- **Prefix**: `A`(Actor), `U`(UObject/Component), `F`(Struct), `E`(Enum), `I`(Interface), `T`(Template)
- **프로젝트 Prefix**: `SS` (SixtySeconds) — ex: `ASSCharacter`, `USSAttributeSet`
- **파일 위치**: 기능 도메인별 서브폴더 유지
- **UPROPERTY 노출**: 에디터 노출 필요한 것만 `EditAnywhere`/`BlueprintReadWrite`
- **한국어 주석 허용** (내부 프로젝트)
- **헤더 include 순서**: 자신의 헤더 → UE 헤더 → 서드파티 (IWYU 준수)

---

## AI 파이프라인 (.claude/)
| 리소스 | 역할 |
|--------|------|
| `agents/ue-architect.md` | 시스템 설계, 모듈 구조, GAS 아키텍처 결정 |
| `agents/ue-implementer.md` | C++/Blueprint 구현, 파일 작성, 빌드 오류 수정 |
| `commands/ue-feature.md` | `/ue-feature` 커맨드: 새 기능 스캐폴딩 |

새 기능 추가 시 워크플로우:
1. `/ue-feature <기능명>` 으로 설계 시작
2. `ue-architect` 에이전트가 구조 결정
3. `ue-implementer` 에이전트가 파일 작성
