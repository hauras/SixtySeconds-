# /ue-feature

새 게임 기능을 설계하고 스캐폴딩합니다.

## 사용법
```
/ue-feature <기능명> [설명]
```

## 예시
```
/ue-feature CompanionRobotReveal 동료가 로봇임을 밝히는 이벤트 시스템
/ue-feature RadiationSystem 방사능 스탯 피해 및 상태이상 처리
/ue-feature ShelterEventSystem 은신처 턴제 이벤트 분기 시스템
```

---

## 워크플로우

다음 순서로 진행하십시오:

### 1단계 — 요구사항 정리
`$ARGUMENTS`를 분석하여 다음을 파악합니다:
- 이 기능이 속하는 **페이즈** (스크램블 / 은신처 / 공통)
- **USSCharacterStats 연동 필요 여부** (스탯 수치 변경 시)
- **USSStatusComponent 연동 필요 여부** (상태이상 관련 시)
- **UI 노출 필요 여부**
- **데이터 주도 설계 필요 여부** (DataTable, DataAsset)

### 2단계 — 아키텍처 설계 (ue-architect 에이전트)
`ue-architect` 에이전트를 호출하여:
- 필요 클래스 목록 및 역할 정의
- 파일 경로 목록 확정
- Blueprint 노출 범위 결정

### 3단계 — 구현 (ue-implementer 에이전트)
`ue-implementer` 에이전트를 호출하여:
- 헤더/소스 파일 생성
- Build.cs 의존성 업데이트 (필요 시)

### 4단계 — 리뷰 (ue-reviewer 에이전트)
`ue-reviewer` 에이전트를 호출하여:
- UE5 컨벤션 및 프로젝트 규약 준수 검토

### 5단계 — 빌드 (ue-builder 에이전트)
`ue-builder` 에이전트를 호출하여:
- 빌드 실행 및 에러 수정

### 6단계 — 에디터 자동화 (ue-editor-automation 에이전트)
`ue-editor-automation` 에이전트를 호출하여:
- Blueprint 에셋 생성 Python 스크립트 작성
- DataTable 생성 스크립트 작성 (해당 시)
- 실행 방법 안내 (에디터 콘솔 명령어 제공)

### 7단계 — 완료 리포트
완료 후 다음을 출력합니다:
```
## 완료: <기능명>

### 생성된 파일
- [ ] Source/.../Foo.h
- [ ] Source/.../Foo.cpp

### 다음 단계 (수동 작업 필요)
- Blueprint 에셋 생성: ...
- DataTable 행 추가: ...
- 에디터에서 테스트: ...
```
