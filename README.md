# StatCheck

Unreal Editor에서 버튼을 눌러 현재 씬의 최적화 후보를 CSV로 기록하는 C++ 플러그인입니다.

이 프로젝트는 단순한 기능 구현보다, **Codex를 이용한 하네스 엔지니어링 기반 개발 흐름**을 포트폴리오로 보여주는 것을 목표로 합니다. 기능을 바로 크게 만들기보다, 구조 분석, 검증 자동화, 역할별 문서화, 성능 문제 기록, 배포 기준 정리를 먼저 세우고 그 위에서 플러그인을 확장했습니다.

## 핵심 요약

- Unreal Engine C++ Editor 플러그인
- `Window > StatCheck` 메뉴에서 별도 Editor 탭 실행
- 실시간 추적 대신 버튼 기반 진단 방식 채택
- FPS, FrameTime, DrawCall, Actor 수, Component 수, 선택 Actor 비용 등을 CSV로 저장
- Actor Tick, Component 분포, Top Actor 후보, 변경 전후 비교 리포트 제공
- 다른 Unreal 프로젝트에 `Plugins/StatCheck` 폴더만 복사해 사용할 수 있도록 배포 구조 정리
- 개발 과정에서 하네스 엔지니어링을 적용해 검증 가능한 단위로 기능 확장

## 왜 만들었나

Unreal 프로젝트를 최적화할 때 초보자는 다음 질문에서 자주 막힙니다.

- 지금 FPS가 낮은 이유가 DrawCall 때문인지 Actor Tick 때문인지 모르겠다.
- 어떤 Actor나 Component부터 봐야 할지 모르겠다.
- `stat fps`, `stat unit`, Profiler를 켜도 결과를 포트폴리오 자료로 남기기 어렵다.
- 실시간 모니터링 UI를 만들면 오히려 플러그인 자체가 성능을 떨어뜨릴 수 있다.

StatCheck는 이 문제를 “항상 켜져 있는 모니터”가 아니라 **필요할 때만 기록하는 진단 도구**로 풀었습니다.

## 주요 기능

### Snapshot 로그

`Save Snapshot Log` 버튼을 누르면 현재 타이밍의 주요 최적화 지표를 한 번 수집해 CSV로 저장합니다.

기록 항목:

- FPS
- FrameTime
- GameThread 근사값
- DrawCall
- 메모리 사용량
- 현재 에디터 뷰포트 기준 Visible Actor 수
- 전체 Actor 수
- Hidden Actor 수
- Tick 활성 Actor 수
- Primitive Component 수
- StaticMesh Component 수
- SkeletalMesh Component 수
- Light Component 수
- Material Slot 수
- StaticMesh 기준 대략 Triangle 수
- 선택 Actor 비용 proxy

### 10초 기록

`Record 10s Log` 버튼을 누르면 10초 동안 1초 간격으로 Snapshot을 기록합니다.

실시간 추적 기능이 아니라, 사용자가 명시적으로 기록을 시작했을 때만 임시 샘플링합니다.

### 최적화 리포트

다음 리포트를 CSV로 저장할 수 있습니다.

- `Actor Tick Report`: Tick이 켜진 Actor 목록
- `Component Report`: Component 타입별 개수
- `Selected Actor Report`: 선택 Actor의 Component 상세 정보
- `Top Actors Report`: 무거운 Actor 후보 상위 50개
- `Compare Baseline`: 최적화 전후 Snapshot 차이

### Unreal 내부 도움말

StatCheck 창 상단의 `?` 버튼을 누르면 사용 방법이 Unreal 팝업으로 표시됩니다.

## 사용 방법

1. Unreal Editor에서 프로젝트를 엽니다.
2. 상단 메뉴에서 `Window > StatCheck`를 선택합니다.
3. 현재 상태를 저장하려면 `Save Snapshot Log`를 누릅니다.
4. 10초간 기록하려면 `Record 10s Log`를 누릅니다.
5. Tick, Component, Top Actor 리포트 버튼으로 세부 후보를 저장합니다.
6. 최적화 전 `Set Baseline`, 최적화 후 `Compare Baseline`을 눌러 전후 비교를 남깁니다.

CSV 로그는 현재 Unreal 프로젝트 기준으로 아래 폴더에 저장됩니다.

```text
Saved/StatCheck/Logs
```

## 다른 프로젝트에 설치하는 방법

이 저장소에서 공유해야 하는 기준 폴더는 아래 하나입니다.

```text
Plugins/StatCheck
```

대상 Unreal 프로젝트에는 다음 구조로 복사합니다.

```text
TargetProject/
  Plugins/
    StatCheck/
      StatCheck.uplugin
      Source/
      Resources/
      사용설명.md
```

복사 후 대상 프로젝트를 열고 플러그인을 빌드하면 됩니다.

자세한 사용 설명은 플러그인 내부의 문서를 확인합니다.

```text
Plugins/StatCheck/사용설명.md
```

## 저장소 구성

이 저장소는 외부 프로젝트에서 바로 가져다 쓸 수 있도록 배포에 필요한 파일만 추적합니다.

```text
Plugins/StatCheck
  StatCheck.uplugin
  사용설명.md
  Resources/
    Icon128.png
  Source/
    StatCheck/
      Runtime 모듈
    StatCheckEditor/
      Editor 메뉴, 탭, Slate UI, 리포트 수집
```

Git에서 제외한 항목:

- 호스트 테스트 프로젝트 파일
- `Binaries`
- `Intermediate`
- `Saved`
- `DerivedDataCache`
- IDE 설정 파일
- 실험용 이미지 리소스

## 기술 구조

### Runtime 모듈: `StatCheck`

Runtime 모듈은 에디터에 종속되지 않는 기본 성능 데이터 구조와 판정 로직을 담당합니다.

주요 역할:

- `FStatCheckPerformanceSnapshot`
- FPS, FrameTime, DrawCall, Memory 값 수집
- `Good`, `Warning`, `Critical` 상태 판정
- Runtime 모듈 로드 테스트

### Editor 모듈: `StatCheckEditor`

Editor 모듈은 Unreal Editor 전용 기능을 담당합니다.

주요 역할:

- `Window > StatCheck` 메뉴 등록
- Nomad Tab 생성
- Slate 기반 `SStatCheckPanel` UI
- 버튼 기반 CSV 로그 저장
- Actor, Component, 선택 Actor, Top Actor 후보 리포트 수집
- Editor 모듈 로드 테스트

## 하네스 엔지니어링 적용 방식

이 프로젝트에서 하네스 엔지니어링은 두 층으로 적용했습니다.

### 1. Codex 오케스트레이션 하네스

Codex를 단순 코드 생성 도구로 쓰지 않고, 작업을 잃지 않기 위한 운영 체계로 사용했습니다.

적용한 방식:

- 프로젝트 구조를 먼저 읽고 `.uplugin`, `Source`, 모듈 구조를 정리
- 기능 명세, 설계, 검증 기준, 의사결정을 문서로 분리
- “실시간 추적은 비용이 크다”는 판단을 기록하고 버튼 기반 진단 방식으로 방향 전환
- 작업마다 다음 단계와 검증 기준을 명확히 한 뒤 구현
- 포트폴리오 문서와 실제 플러그인 기능을 함께 발전

이 방식은 여러 Codex 스레드를 병렬로 운영할 때도 유효합니다.

예시 역할 분리:

- Orchestrator: 전체 목표, 현재 단계, 완료 기준 관리
- Feature Spec: 기능 요구사항 관리
- Collector Thread: Stat 수집 로직 구현
- UI Thread: Slate 패널과 버튼 구현
- Validation Thread: 빌드, 테스트, 회귀 확인
- Docs Thread: 사용 설명과 포트폴리오 기록 정리

### 2. 빌드/테스트 검증 하네스

기능 구현 전후로 반복 검증할 수 있는 로컬 하네스를 구성했습니다.

개발 과정에서 사용한 검증 단위:

- Unreal Engine 경로와 프로젝트 파일 확인
- Editor 타겟 빌드
- Unreal Automation Test 실행
- Runtime 모듈 로드 테스트
- Editor 모듈 로드 테스트
- Collector/Evaluator 테스트

개발 흐름은 다음 순서를 따랐습니다.

```text
구조 분석
→ 최소 하네스 작성
→ 모듈 로드 테스트
→ Editor 탭 추가
→ 기능 단위 구현
→ Verify 실행
→ 성능 문제 기록
→ 설계 수정
→ 배포 기준 정리
```

## 중요한 설계 결정

### 실시간 추적을 제거한 이유

초기에는 창을 열어 두면 값이 계속 갱신되는 구조를 실험했습니다.

하지만 StatCheck 자체가 FPS에 영향을 줄 수 있다는 점을 확인했고, 성능 진단 도구가 측정 대상의 성능을 크게 흔들면 안 된다고 판단했습니다.

그래서 현재 구조는 다음 원칙을 따릅니다.

- 창을 열어 둔 것만으로 반복 수집하지 않는다.
- 버튼을 눌렀을 때만 비용을 지불한다.
- 10초 기록도 사용자가 명시적으로 시작했을 때만 동작한다.
- 파일 쓰기는 샘플링 중 매번 하지 않고, 메모리에 모았다가 마지막에 저장한다.

### 이미지 애니메이션을 제거한 이유

초기 아이디어에는 성능 상태에 따라 2D 동물 스프라이트가 바뀌는 기능이 있었습니다.

하지만 실제 사용 중 이미지 표시가 FPS에 부담을 줄 수 있음을 확인했고, 플러그인의 핵심 가치를 “보여주는 재미”보다 “측정 도구로서의 신뢰성”에 두기로 했습니다.

이 결정은 포트폴리오 관점에서도 의미가 있습니다.

기능을 더하는 것뿐 아니라, 측정 결과를 근거로 기능을 줄이는 판단도 제품 품질 개선이기 때문입니다.

## 포트폴리오 관점의 가치

이 프로젝트는 다음 역량을 보여줍니다.

- Unreal Engine C++ 플러그인 구조 이해
- Runtime 모듈과 Editor 모듈 분리
- Slate 기반 Editor UI 구현
- Unreal Automation Test 기반 최소 회귀 테스트 구성
- 성능 진단 도구의 오버헤드 문제 인식
- 버튼 기반 관측 도구 설계
- CSV 로그로 최적화 근거를 남기는 워크플로우
- Codex를 이용한 문서 중심 하네스 엔지니어링
- 배포 가능한 플러그인 폴더 기준 정리

## 검증 상태

마지막 검증에서 아래 항목을 통과했습니다.

- `StatCheckPluginEditor Win64 Development` 빌드 성공
- Unreal Automation Test 5개 통과
- 전체 Verify 하네스 통과

## 한계와 다음 개선 방향

현재 StatCheck는 “가볍게 후보를 찾는 도구”입니다.

다음 개선 후보:

- RenderThread/GPU 값 수집 경로 조사
- Unreal Insights 연동
- Top Actor 리포트의 Cost Score 기준 고도화
- CSV 비교 결과를 Editor 안에서 표로 표시
- 프로젝트별 임계값 설정 파일 추가
- 로그 파일 열기 버튼 추가

## 라이선스

현재 별도 라이선스 파일은 없습니다. 공개 배포 또는 협업 전에는 라이선스 정책을 추가하는 것을 권장합니다.
