# Sleepless Knight (잠 못 드는 기사) - 가제: 파티 마왕 민원기

UE 5.7 / C++ + GAS / 1인 개발 / PC(Steam) 싱글 플레이. 기획서 원본: `액션 게임 기획서.docx`, `설정 노트.docx` (스토리 구상서).

## 게임 요약
- 밝은 개그 카툰풍 3인칭 스테이지형 핵앤슬래시. 주인공 레오 1명 고정, 날 없는 둔기 1종.
- 적은 죽지 않고 곯아든다(KO 카운트 = "재운 마물 수"). 보스마다 음악/춤이 곧 공격 패턴.
- 6스테이지(성 밖 3 + 성 안 3), 스테이지당 15분, 총 약 2시간. 보스: 세이렌 > 골렘 > 메두사 > 엘프 > 미노타우르스 > 마왕.
- 성장은 "보스를 이기면 새 기술" 하나뿐. 레벨/재화 없음 (콤보 해금, 보스 필살기, 체력 증가 베개).

## 전투
- 입력: 약공격(최대 4단), 강공격(단독 또는 약 N타 후 파생), 회피(시작 직후 무적), 필살기(게이지), 락온.
- 파생: 강 단독=내려치기, 약1>강=올려치기, 약2>강=휘두르기, 약3>강=홈런 스윙.
- 히트스톱 약 0.03s / 강·막타 0.08s (시작값, 프로토타입에서 조정). 스쿼시 앤 스트레치, 만화 글자 이펙트.
- 적 동시 공격은 1~2마리로 제한 (C++ 매니저 하나로 관리). 한 전투 5~12마리.

## 기술 방침
- 핵심 로직(전투 판정, 적 AI, 스폰, 성장 데이터)은 C++, 수치/연출/UI/컷신은 블루프린트로 노출해 조정.
- GAS 도입: 콤보 파생/필살기/회피는 GameplayAbility, 체력/필살 게이지는 AttributeSet. 리플리케이션 없음.
- 공격 판정: 애님 몽타주 + 노티파이, 락온 방향 보정은 모션 워핑.
- 적 AI: 일반=C++ 상태 머신, 특수/보스=비헤이비어 트리 + C++ 태스크.
- 보스 패턴은 음악 박자 이벤트(Quartz 등)에 묶는다.
- 렌더링: 포스트 프로세스 외곽선 + 셀 셰이딩, 초반에 확정해 모든 에셋에 공통 적용.
- 이펙트는 나이아가라 + 2D 스프라이트, 컷신은 시퀀서.

## 마일스톤 (현재: 1단계)
1. 전투 프로토타입 (회색 박스 레오, 둔기 콤보, 회피, 일반 적 1종, 외곽선/셀 셰이딩 테스트). 완료 기준: 회색 박스 상태로도 때리는 게 재밌다.
2. 아트 파이프라인 > 3. 수직 슬라이스(스테이지 1 + 세이렌) > 4. 콘텐츠 제작 > 5. 폴리싱/출시.
기준을 통과하기 전에는 다음 단계로 넘어가지 않는다. 밀리면 구역 변주, 특수 적 종류, 리믹스 페이즈, 컷신 순으로 줄인다.

## 코드 구조
- 모듈 `SleeplessKnight`, 클래스 접두사 `SK`. 폴더: `Public|Private/{Characters,GAS,...}`.
- `ASKCharacter`: 레오. ASC를 직접 소유, Enhanced Input은 코드에서 생성(입력 에셋 없음).
- `USKAttributeSet`: Health, MaxHealth, UltGauge, MaxUltGauge.
- `ASKGameMode`: 기본 폰 = ASKCharacter. 프로토타입용으로 BeginPlay에서 PlayerStart 주변에 적을 원형 배치하고 "재운 수" 카운터를 화면에 표시.
- `USKGA_LightAttack`: 4단 약공격 콤보 (FSKComboStep 배열로 타이밍/데미지/넉백/히트스톱 조정). 단계는 타이머 기반, 몽타주 생기면 노티파이로 교체. 입력은 `ASC->AbilityLocalInputPressed(SKAbilityInput::LightAttack)`로 시작/버퍼링.
- `ASKEnemy`: 체력 0이 되면 죽지 않고 곯아떨어짐. 스쿼시 앤 스트레치(감쇠 스프링)와 Zzz 텍스트는 임시 구현.
- 히트스톱: `ASKCharacter::RequestHitStop` (글로벌 타임 딜레이 0.02 + 실시간 티커로 복구).
- 맵: `Content/Maps/BasicMap` (기본 맵으로 지정됨).
- `ASKMovingPlatform` (`World/`): 시작점과 `TargetOffset` 사이를 왕복. 블루프린트 자식 `Content/Blueprints/BP_MovingPlatform`.
- `USKOptionsMenu` (`UI/`): 우상단 톱니 버튼 + 옵션 패널(사운드/화면/저장/불러오기/종료). 위젯 트리를 코드로 생성(UMG 에셋 없음). 메뉴 키는 1 (Esc는 PIE 종료라서 임시), 열면 일시정지. 톱니는 `USKGearIcon`(Slate로 직접 그림).
- `USKSaveGame`: 슬롯 `QuickSave`. 레오 위치/체력/필살 게이지/볼륨. 적 상태는 저장 안 함.
- 에디터 Python으로 블루프린트 에셋을 만들 때는 `.uproject`에 `PythonScriptPlugin`을 임시로 켜고 `UnrealEditor-Cmd.exe -ExecutePythonScript=...`로 실행한 뒤 되돌린다.

## 빌드
```
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" SleeplessKnightEditor Win64 Development -Project="<절대경로>\SleeplessKnight.uproject" -WaitMutex -NoHotReload
```
에디터가 열려 있으면 핫리로드/Live Coding과 충돌하니 닫고 빌드하거나 에디터에서 Ctrl+Alt+F11.

## 미정 사항 (기획서 기준)
둔기 종류(칼집 검 / 뿅망치 / 베개 메이스), 동료 여부, 아트 최종 확정, 시나리오 탭 반영.
