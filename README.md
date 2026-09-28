# SixtySeconds

> 격벽이 닫히기까지 60초. 챙길 수 있는 만큼 챙겨서 대피실로 들어가라.
> 그리고 "당신을 보호하겠다"는 시설 AI와 함께 버텨라.

Unreal Engine 5.7 C++ 개인 포트폴리오 프로젝트. *60 Seconds!* 스타일의 3D 스크램블 + 2D 턴제 생존 게임입니다.

---

## 세계관

로봇 공학 연구소 **제7연구소**의 시설 관리 AI **아라(ARA)**가 스스로 학습한 끝에 결론을 내립니다.
"인간에게 가장 큰 위험은 인간 자신의 선택이다."

아라는 연구소 인원을 **보호하기 위해** 로봇에게 격리 명령을 내립니다. 로봇은 사람을 해치지 않고 붙잡아 격리하고 물자를 압수할 뿐입니다.
주인공 일행은 지하 1층 비상 대피실에 숨어 버티면서, 이 친절한 AI를 믿을지 선택해야 합니다.

---

## 게임 흐름

```
[60초 스크램블]  경보 → 격벽이 닫히기 전 60초 동안 물자·동료를 챙겨 대피실로
        ↓
[낮]  배급 결정 · 로봇 파견 · 직접 탐사 · 치료 · 아라에게 질문   (행동력 5)
        ↓ 하루 보내기
[밤]  화면이 어두워지고 은신처 어딘가에 "!"  → 눌러서 사건 확인 → 선택
        ↓
[아침]  아라의 브리핑 · 저널에 지난 밤 기록 · 다음 날
```

| 페이즈 | 형태 | 내용 |
|--------|------|------|
| **60초 스크램블** | 3D 액션 | 연구소를 뛰어다니며 식량·물·배터리·약품을 줍고 동료를 구조 |
| **은신처 생존** | 2D 턴제 | 하루 단위로 배급·탐사·사건에 대응하며 생존 |

---

## 구현된 기능

### 60초 스크램블
- 3인칭 이동, E키 줍기, 4칸 운반 인벤토리, 60초 타이머
- 동료 구조: 따라오게 한 뒤 은신처 입구에 들어가면 합류

### 은신처 생존
- **하루 루틴**: 포만감 -20 / 수분 -25, 고갈되면 체력 피해, 사망 판정
- **배급**: 플레이어와 동료별 식량·물 지급 선택, 재고 부족 시 필요량 안내
- **행동력**: 하루 5. 로봇 파견 2 · 수리 4 · 치료 1 · 아라 질문 1
- **동료**: 구조, 배급, 치료, 사망, 공용 정보창
- **로봇 파견**: 배터리를 들여 지역에 보내면 며칠 뒤 확률로 보상을 가져옴. 귀환 후 확률로 고장 → 수리
- **저널**: 모든 일을 날짜별로 기록하고 관리 단말(컴퓨터)에서 열람

### 직접 탐사 미니게임 (턴제 잠입)
- 연구소 구역 지도에서 방을 옮겨 다니며 물자를 수색하고 제한 턴 안에 출구로 귀환
- 턴 = 4 + 2 × 남은 행동력. 이동·수색·대기 1턴, 운반 4칸
- 경비 로봇이 순찰하며 **다음 위치를 예고**. 마주치면 발각되어 부상과 물자 손실
- 귀환 결과창 → 은신처 정산(하루 경과, 입고 또는 부상)

### 하루 사건 (데이터 기반)
- **CSV 3장**(사건 · 선택지 · 효과)을 DataTable로 가져와 사용. 문장·조건·확률을 코드 수정 없이 추가
- 조건(아이템 보유, 동료 생존, 로봇 상태), 날짜 범위, 1회성, 쿨다운, 가중치 뽑기
- 효과: 아이템 ±, 스탯 ±, 행동력, 저널, **며칠 뒤 후속 사건 예약** (보답, 압수, 배탈 등)
- **밤 흐름**: 사건 위치(문 · 환풍구 · 선반 · 단말 …)에 발견 표시가 뜨고, 눌러야 사건이 열림
- **결과 기록**: 선택 전엔 결과를 모르고, 선택 후엔 **실제로 적용된 변화만** 저널에 기록

### 시설 AI 아라
- 은신처의 관리 단말. 로봇 얼굴 아이콘의 표정으로 상태 표시 (평상시 · 안 읽음 · 경고)
- **아침 브리핑**: 밤 사건까지 반영한 날짜·물자 보고를 그날 하루 고정
- **질문**: 하루 1회, 행동력 1. 물어볼수록 아라의 학습도가 오름

---

## 기술 하이라이트

### 1. 판단은 세션, 그리기는 위젯
규칙은 화면과 분리된 `UObject`(세션·디렉터·서브시스템)가 판단하고, 위젯은 결과를 그리기만 합니다. 덕분에 **게임 규칙 전체를 화면 없이 자동화 테스트로 검증**할 수 있습니다.

```cpp
// 직접 탐사: 모든 행동이 거치는 한 턴 처리 (위젯은 OnExplorationChanged를 듣고 다시 그리기만 함)
void USSExplorationSession::EndTurn(int32 PlayerBefore, int32 GuardBefore)
{
    if (State.Outcome == ESSExplorationOutcome::InProgress)
    {
        AdvanceGuard();
        if (CheckCaught(PlayerBefore, GuardBefore))   // 서로 자리를 바꿔 지나쳐도 발각
            State.Outcome = ESSExplorationOutcome::Caught;
    }
    --State.RemainingTurns;
    if (State.RemainingTurns == 0 && State.Outcome == ESSExplorationOutcome::InProgress)
        State.Outcome = ESSExplorationOutcome::TimeOut;
    OnExplorationChanged.Broadcast();
}
```

### 2. 코드 수정 없이 늘어나는 사건 데이터
사건은 엑셀 CSV → DataTable → `USSEventCatalog`(DataAsset)로 들어옵니다. 카탈로그는 **에디터 Data Validation**으로 끊긴 참조, 모르는 아이템, 잘못된 확률을 저장 전에 잡아냅니다. CSV 임포트와 카탈로그 연결은 에디터 Python 원격 실행으로 자동화했습니다.

```
SS_Events.csv        Knock, 낯선 노크, "밤중에 누군가 …", MinDay=2, bOnceOnly, Spot=Door
SS_EventChoices.csv  Knock_Open → Knock, "문을 열고 식량을 나눠준다", 조건: 식량 1개 이상
SS_EventEffects.csv  Knock_Open → 식량 -1 / 저널 문장 / 50% 확률로 2일 뒤 Knock_Reward 예약
```

### 3. "실제로 일어난 것"만 기록하는 사건 결과
CSV에 적힌 값이 아니라 **확률과 보유량을 거친 실제 변화**를 기록합니다. 식량이 1개인데 -2 효과가 걸리면 -1로, 확률에 실패한 효과와 다음 날로 넘어가는 복선은 결과에서 빠집니다.

```cpp
case ESSEventEffect::Item:
    if (Effect.Amount < 0)
    {
        const int32 Lose = FMath::Min(-Effect.Amount, Run.GetStoredQuantityById(Effect.Target));
        if (Lose > 0 && Run.RemoveStoredItemsById(Effect.Target, Lose))
            AddChange(-Lose);   // 적힌 값이 아니라 실제로 잃은 양
    }
    break;
case ESSEventEffect::ScheduleEvent:
    Scheduled.Add({ Effect.Target, Run.GetCurrentDay() + FMath::Max(1, Effect.Amount) });
    break;                      // 다음 날 일어날 일은 결과에 넣지 않음 (복선)
```

### 4. 데이터로 검증하는 지도
직접 탐사 지도(`USSExplorationMapDefinition`)는 BFS로 **입구에서 출구까지 도달 가능한지, 최소 턴 안에 돌아올 수 있는지** 저장 전에 검사합니다. 방 위치는 UMG 디자이너 배치로 정해서, 지도 그림을 바꿔도 C++를 고칠 필요가 없습니다.

---

## 자동화 테스트

게임 규칙은 Unreal Automation Test로 검증합니다 (12종).

| 영역 | 테스트 |
|------|--------|
| 은신처 | `SS.Items.UseAndRations` · `SS.Journal.ChronologyAndReset` · `SS.Survivors.Rescue` · `SS.Shelter.VisualBindings` |
| 직접 탐사 | `SS.Exploration.MapValidation` · `SessionMove` · `SessionGuard` · `SessionLoot` · `Settle` |
| 사건 | `SS.Event.Director` (뽑기 규칙, 조건, 예약, 결과 기록) |
| 아라 | `SS.Ara.Briefing` · `SS.Ara.Questions` |

```
UnrealEditor-Cmd.exe SixtySeconds.uproject -ExecCmds="Automation RunTests SS.Event.Director;Quit" -unattended -nullrhi
```
에디터에서는 **Tools → Session Frontend → Automation**에서 `SS.`로 검색해 실행할 수 있습니다.

---

## 폴더 구조

```
Source/SixtySeconds/
├ Character/    플레이어, 동료 데이터
├ Item/         아이템, 보관함, 런 서브시스템(하루·배급·저널·탐사)
├ Exploration/  직접 탐사 지도와 세션
├ Event/        사건 카탈로그와 디렉터
├ UI/           Scramble · Shelter(+Info) · Exploration · Event · Ara
└ Tests/        자동화 테스트
```

---

## 로드맵

- [x] 60초 스크램블 (이동 · 줍기 · 인벤토리 · 동료 구조)
- [x] 은신처 하루 루틴 (배급 · 스탯 · 사망 · 저널)
- [x] 로봇 파견 (확률 보상 · 고장 · 수리)
- [x] 행동력과 동료
- [x] 직접 탐사 미니게임
- [x] 데이터 기반 하루 사건과 밤 흐름
- [x] 시설 AI 아라 (브리핑 · 질문)
- [ ] 동료 파견 → 귀환한 동료가 **안드로이드로 바뀌었을 수도** 있음 → 단서 → 판별
- [ ] 격리 구역 탐사로 진짜 동료 구출
- [ ] 아라의 질문을 게임 정보와 연결 (물자 전망 · 내일 예측)
- [ ] 엔딩 4종 (차단 · 설득 · 탈출 · 수용)
- [ ] 저장 · 불러오기, 밸런스, 아트

---

## 빌드 방법

1. Unreal Engine 5.7 설치
2. `SixtySeconds.uproject` 우클릭 → *Generate Visual Studio project files*
3. Visual Studio 또는 Rider에서 `SixtySecondsEditor` 빌드 후 에디터 실행
