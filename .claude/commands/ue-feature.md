# /ue-feature

새 게임 기능을 설계하고 스캐폴딩합니다.

## 사용법
```
/ue-feature <기능명> [설명]
```

## 예시
```
/ue-feature CompanionRobotReveal 동행자가 로봇임을 밝히는 이벤트 시스템
/ue-feature RadiationAttribute GAS 방사능 Attribute 및 피해 Effect
/ue-feature ShelterEventSystem 은신처 턴제 이벤트 분기 시스템
```

---

## 워크플로우

다음 순서로 진행하십시오:

### 1단계 — 요구사항 정리
`$ARGUMENTS`를 분석하여 다음을 파악합니다:
- 이 기능이 속하는 **페이즈** (스크램블 / 은신처 / 공통)
- **GAS 연동 필요 여부** (상태이상, 속성, 능력 포함 시)
- **UI 노출 필요 여부**
- **데이터 주도 설계 필요 여부** (DataTable, DataAsset)

### 2단계 — 아키텍처 설계 (ue-architect 에이전트)
`ue-architect` 에이전트를 호출하여:
- 필요 클래스 목록 및 역할 정의
- 모듈 배치 결정
- GAS 연동 설계 (해당 시)
- 파일 경로 목록 확정

### 3단계 — 구현 (ue-implementer 에이전트)
`ue-implementer` 에이전트를 호출하여:
- 헤더/소스 파일 생성
- Build.cs 의존성 업데이트
- GameplayTag 등록

### 4단계 — 완료 리포트
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
