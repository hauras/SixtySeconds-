# PostToolUse 훅 — C++ 소스 파일 저장 후 clang-format 적용
# clang-format 없으면 조용히 통과 (빌드 방해 없음)

param(
    [string]$ToolName,
    [string]$ToolInput
)

# Write, Edit 이후만 실행
if ($ToolName -notmatch '^(Write|Edit)$') {
    exit 0
}

# C++ 파일만 대상
if ($ToolInput -notmatch '"file_path"\s*:\s*"([^"]+\.(h|cpp|inl))"') {
    exit 0
}

$filePath = $Matches[1]

# generated.h는 건드리지 않음 (guard-generated.ps1 이 차단하지만 이중 안전장치)
if ($filePath -match '\.generated\.h$') {
    exit 0
}

# clang-format 존재 확인
$clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $clangFormat) {
    exit 0
}

try {
    & clang-format -i $filePath 2>$null
} catch {
    # 포매팅 실패해도 빌드 방해하지 않음
}

exit 0
