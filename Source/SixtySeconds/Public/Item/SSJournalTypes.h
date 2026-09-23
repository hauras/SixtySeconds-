#pragma once

#include "CoreMinimal.h"
#include "SSJournalTypes.generated.h"

UENUM(BlueprintType)
enum class ESSJournalEvent : uint8
{
    Deposit, Rations, DayEnd, Expedition, Robot, ItemUse, Death
};

// 당시 내용을 보존한다. 현재 아이템 수량이나 DA 이름으로 과거 기록을 다시 만들지 않는다.
USTRUCT(BlueprintType)
struct FSSJournalEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 Day = 1;
    UPROPERTY(BlueprintReadOnly) ESSJournalEvent Event = ESSJournalEvent::DayEnd;
    UPROPERTY(BlueprintReadOnly) FText Message;
};
