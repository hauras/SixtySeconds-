#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Companion/SSInvestigation.h"
#include "SSCompanionLines.generated.h"

// ─────────────────────────────────────────────
// 단서 표(SS_Clues) 한 줄. 행 이름이 단서 ID (예: Clue_Terminal_2)
// 장소마다 Stage 1 → 2 → 3 순서로 하나씩 찾음
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSClueRow : public FTableRowBase
{
	GENERATED_BODY()

	// 어느 장소에서 나오는 단서인지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Clue")
	ESSInvestigationSpot Spot = ESSInvestigationSpot::Storage;

	// 그 장소에서 몇 번째로 나오는지 (작은 것부터)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Clue")
	int32 Stage = 1;

	// 단서 제목 (대화창 카드·기록창)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Clue")
	FText Title;

	// 단서 내용. 본 것만 적음 (해석은 플레이어 몫)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Clue", meta=(MultiLine=true))
	FText Text;

	// 기획 메모: 이 단서가 무엇과 이어지는지 (게임에서는 안 씀)
	UPROPERTY(EditAnywhere, Category="SS|Clue", meta=(MultiLine=true))
	FString Purpose;
};

// 동료 공통 대사 종류 (행 이름: 동료ID_종류. 예: Technician_Greet)
UENUM(BlueprintType)
enum class ESSCompanionLine : uint8
{
	Greet,           // 할 얘기가 없을 때 첫마디
	ReportReady,     // 들을 보고가 하나 있을 때
	Backlog,         // 보고가 여러 개 밀렸을 때
	OrderReminder,   // 오늘 밤 조사 장소가 정해져 있을 때 ({Spot})
	OrderAccept,     // 조사 장소를 정해 줬을 때 ({Spot})
	Exhausted,       // 단서를 다 찾은 장소를 또 조사했을 때 ({Spot})
	Testimony,       // B2에서 구출된 뒤 처음 하는 증언 (숨은 진실)
};

// ─────────────────────────────────────────────
// 동료 대사 표(SS_CompanionLines) 한 줄. 행 이름이 열쇠
//   보고: 동료ID_Report_장소_Found / 동료ID_Report_장소_None
//   공통: 동료ID_종류 (ESSCompanionLine 이름)
// 대사 안의 {Spot}은 장소 이름으로 바뀜
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSCompanionLineRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Companion", meta=(MultiLine=true))
	FText Line;
};
