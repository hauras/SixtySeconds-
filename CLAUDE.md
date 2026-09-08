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
- 상태이상 시스템: 커스텀 컴포넌트로 구현 (GAS 미사용)
- 자원 관리, 로봇 침공 이벤트, 생존 결정

---

## 기술 스택
- **Engine**: Unreal Engine 5 (C++ + Blueprint 혼용)
- **Target Platform**: PC (Windows)
- **Build System**: UnrealBuildTool (UBT)
- **GAS**: 미사용 — 싱글플레이어 턴제 구조에 오버킬, 커스텀 컴포넌트로 대체

---

## 아키텍처 개요

### 모듈 구조
```
Source/
└── SixtySeconds/          # 단일 게임 모듈
    ├── Core/              # GameMode, GameState, GameInstance
    ├── Phase/             # 페이즈 전환 (스크램블 ↔ 은신처)
    ├── Character/         # 플레이어 캐릭터, 스탯, 상태이상
    ├── Companion/         # 동행자 시스템 (로봇 정체 포함)
    ├── Shelter/           # 은신처 상태, 이벤트
    ├── Item/              # 아이템 데이터, 인벤토리
    └── UI/                # HUD, 위젯
```

### 핵심 시스템 클래스
| 클래스 | 역할 |
|--------|------|
| `ASSGameMode` | 페이즈 전환, 전체 게임 흐름 제어 |
| `USSPhaseManager` | 스크램블/은신처 전환 로직 |
| `USSCharacterStats` | HP·허기·갈증·방사능·정신력 수치 관리 컴포넌트 |
| `USSStatusComponent` | 상태이상 추가/제거/틱 관리 컴포넌트 |
| `USSCompanionSubsystem` | 동행자 관리, 로봇 여부 추적 |
| `USSShelterManager` | 은신처 상태, 날짜, 이벤트 |

---

## 상태이상 설계 원칙
- `ESSStatusType` Enum으로 상태이상 종류 정의
- `FSSStatusEffect` Struct: Type, Duration, TickInterval, TickDamage
- `USSStatusComponent`가 `TArray<FSSStatusEffect>` 보유 및 매 Tick 처리
- 상태이상 추가/제거는 항상 `USSStatusComponent`를 통해 (직접 스탯 조작 금지)
- 은신처 턴제에서는 TimerManager 대신 `OnTurnPassed()` 이벤트로 처리

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
| `agents/ue-architect.md` | 시스템 설계, 클래스 구조, 커스텀 컴포넌트 아키텍처 결정 |
| `agents/ue-implementer.md` | C++/Blueprint 구현, 파일 작성 |
| `agents/ue-builder.md` | 빌드 실행 및 컴파일 에러 수정 |
| `agents/ue-reviewer.md` | UE5 코딩 컨벤션 및 프로젝트 규약 리뷰 |
| `agents/ue-doc-writer.md` | CLAUDE.md 업데이트 및 내부 문서화 |
| `agents/ue-editor-automation.md` | UE5 Python API로 Blueprint/DataTable 에셋 자동 생성 |
| `commands/ue-feature.md` | `/ue-feature` 커맨드: 새 기능 스캐폴딩 (6단계 자동화) |
| `hooks/guard-generated.ps1` | *.generated.h / Intermediate / Binaries 편집 자동 차단 |
| `hooks/format-source.ps1` | C++ 저장 후 clang-format 자동 적용 |

새 기능 추가 시 워크플로우 (`/ue-feature` 자동 실행):
1. `ue-architect` — 클래스 구조 설계
2. `ue-implementer` — C++ 파일 작성
3. `ue-reviewer` — 코드 리뷰
4. `ue-builder` — 빌드 및 에러 수정
5. `ue-editor-automation` — Blueprint/DataTable Python 스크립트 생성
6. `ue-doc-writer` — 문서화
