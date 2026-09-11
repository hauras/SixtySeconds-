# SixtySeconds

> 로봇 침공 생존 게임 — 60초 안에 필요한 것들을 쓸어담고, 은신처에서 버텨라.

Unreal Engine 5 C++ 개인 프로젝트.

---

## 게임 개요

| 페이즈 | 형태 | 내용 |
|--------|------|------|
| **60초 스크램블** | 3D 액션 | 제한 시간 내 맵을 뛰어다니며 식량·물·배터리·의약품 수집 |
| **은신처 생존** | 2D 턴제 | 수집한 자원으로 날짜별 배급 결정, 로봇 파견, 생존 관리 |

---

## 구현된 기능

### 스크램블 페이즈
- 3D 캐릭터 이동 및 카메라
- E키 아이템 줍기 + 인벤토리 관리
- 60초 타이머 → 은신처 페이즈 전환

### 은신처 페이즈
- **하루 루틴**: 날짜 경과 시 포만감 -20 / 수분 -25 감소, 고갈 시 체력 패널티
- **배급 시스템**: 식량·물 지급 여부를 체크박스로 선택 후 하루 넘기기
- **아이템 사용**: 물(수분+50), 식량(포만감+40), 의약품(체력+30), 배터리(탐사 자원)
- **로봇 탐사 시스템**
  - 배터리 소모 후 로봇 파견
  - 지정 일수 경과 후 자동 귀환 및 보상 입고
  - 중복 파견 차단, 배터리 부족 검사
- **HUD**: 체력·포만감·수분·보관 수량(물·식량·배터리) 실시간 표시

---

## 기술 스택

- **Engine**: Unreal Engine 5 (C++ only, Blueprint Event Graph 미사용)
- **Platform**: Windows PC
- **Architecture**
  - `USSRunSubsystem` (GameInstanceSubsystem) — 은신처 상태·아이템·탐사 데이터 중앙 관리
  - `USSItemDefinition` / `USSExpeditionDefinition` — DataAsset 기반 데이터 드리븐 설계
  - Dynamic Multicast Delegate (`FOnRobotReturned`) — 탐사 귀환 이벤트 전파
  - `UUserWidget` + BindWidget — HUD 및 탐사 위젯

---

## 아키텍처 하이라이트

**비용 중복 검사 버그 수정**

탐사 비용 배열에 동일 ItemId가 여러 번 들어와도 개별 검사는 통과해버리는 문제를,  
`TMap<FName, int32>` 로 ItemId별 합산 후 한 번에 검사하는 방식으로 수정.

```cpp
TMap<FName, int32> RequiredTotals;
for (const FSSItemStack& CostStack : Expedition->Cost)
    RequiredTotals.FindOrAdd(CostStack.Item->ItemId) += CostStack.Quantity;

for (const auto& Pair : RequiredTotals)
    if (GetStoredQuantityById(Pair.Key) < Pair.Value)
        return ESSExpeditionStartResult::NotEnoughBattery;
```

**역방향 순회 삭제**

`TArray`에서 조건부 삭제 시 인덱스 밀림을 방지하기 위해 역방향 순회 사용.

```cpp
for (int32 Index = StoredItems.Num() - 1; Index >= 0 && Remaining > 0; --Index)
{
    // ...
    if (Stack.Quantity == 0)
        StoredItems.RemoveAt(Index);
}
```

---

## 로드맵

- [x] 60초 스크램블 (이동·줍기·인벤토리)
- [x] 은신처 하루 루틴 (스탯 감소·사망)
- [x] 아이템 사용 시스템
- [x] 로봇 탐사 시스템 (파견·귀환·보상)
- [ ] 탐사 결과 다양화 (성공률·부분 획득·실패)
- [ ] 로봇 고장·수리 시스템
- [ ] 단서·CCTV 획득
- [ ] 저장·불러오기

---

## 빌드 방법

1. Unreal Engine 5 설치
2. `SixtySeconds.uproject` 우클릭 → *Generate Visual Studio project files*
3. Visual Studio에서 빌드 후 에디터 실행
