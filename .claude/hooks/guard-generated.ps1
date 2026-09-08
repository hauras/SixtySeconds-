# PreToolUse 훅 — 엔진 생성/관리 파일 편집 차단
# Claude가 건드리면 안 되는 파일을 Write/Edit 시도 시 즉시 차단

param(
    [string]$ToolName,
    [string]$ToolInput
)

$blockedPatterns = @(
    '\.generated\.h$',
    '[/\\]Intermediate[/\\]',
    '[/\\]Binaries[/\\]',
    '[/\\]DerivedDataCache[/\\]',
    '[/\\]Saved[/\\]',
    '\.uasset$',
    '\.umap$',
    'Engine[/\\]Source[/\\]'
)

# Write, Edit, MultiEdit 도구만 검사
if ($ToolName -notmatch '^(Write|Edit|MultiEdit)$') {
    exit 0
}

# file_path 파싱
if ($ToolInput -match '"file_path"\s*:\s*"([^"]+)"') {
    $filePath = $Matches[1]
    foreach ($pattern in $blockedPatterns) {
        if ($filePath -match $pattern) {
            Write-Error "BLOCKED: '$filePath' 는 엔진/빌드 관리 파일입니다. Claude는 이 파일을 편집할 수 없습니다."
            exit 2
        }
    }
}

exit 0
