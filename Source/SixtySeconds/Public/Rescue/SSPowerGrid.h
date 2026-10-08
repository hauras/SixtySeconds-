
#pragma once

#include "CoreMinimal.h"

namespace SSPowerSide
{
	constexpr uint8 North = 1;
	constexpr uint8 East = 2;
	constexpr uint8 South = 4;
	constexpr uint8 West = 8;
}

struct FSSPowerAlarm
{
	int32 Cell = INDEX_NONE;
	uint8 Side = 0;
};
struct SIXTYSECONDS_API FSSPowerGrid
{
	int32 Width = 5;
	int32 Height = 4;

	// 칸마다 지금 열린 면 (플레이어가 돌리는 것)
	TArray<uint8> Tiles;

	TArray<uint8> Solution;

	int32 SourceCell = INDEX_NONE;
	int32 LockCell = INDEX_NONE;

	TArray<FSSPowerAlarm> Alarms;

	// 풀 수 있는 퍼즐을 만들고 섞음. 같은 시드 = 같은 퍼즐
	// MaxSolveTurns: 섞은 뒤 정답 경로를 맞추는 데 드는 회전 수 상한 (회전 예산 안에 풀리게)
	void Generate(int32 InWidth, int32 InHeight, int32 AlarmCount, FRandomStream& Random, int32 MaxSolveTurns = MAX_int32);

	// 시계 방향 90° (static: 칸 하나의 비트만 보면 돼서 퍼즐이 필요 없음)
	static uint8 RotateClockwise(uint8 Mask);

	void RotateTile(int32 Cell);

	// 전력이 흐르는 칸 번호들
	TSet<int32> ComputePowered() const;

	// 잠금까지 전력이 이어졌는지
	bool IsUnlocked() const;

	// 지금 전력이 닿아 있는 경보 단자 번호들 (Alarms 배열 기준)
	TArray<int32> GetHotAlarms() const;

	// 정답 배선에서 전원 칸 → 잠금 칸 경로 위의 칸들 (트리라 길은 하나)
	TArray<int32> FindSolutionPath() const;

	// 그 칸을 시계 방향으로 몇 번 돌려야 정답 모양이 되나 (0~3, 대칭 타일은 최소 횟수)
	int32 TurnsToSolve(int32 Cell) const;

	// 정답 경로 칸을 모두 맞추는 데 드는 회전 수 (경로 밖 칸은 안 맞춰도 풀림)
	int32 CountTurnsToSolve() const;
};
