# scripts/

이 폴더의 파일들은 전부 **PC에서만 실행되는 개발/테스트 도구**다. `idf.py build`가
이 폴더를 컴파일 대상으로 보지 않으므로, 여기 뭘 추가해도 ESP32 펌웨어(`main/`)
자체에는 아무 영향이 없다 — "제품 기능 추가"가 아니라 "공구함에 도구 추가"에 가깝다.

| 파일 | 용도 | 실행 환경 |
|---|---|---|
| `setup_env.ps1` | ESP-IDF 개발 환경을 현재 셸에 활성화 (`. .\scripts\setup_env.ps1`로 dot-sourcing 필수 — 새 프로세스로 띄우면 그 프로세스 끝나는 순간 환경변수 같이 사라짐) | PowerShell |
| `profile_snippet.ps1` | `$PROFILE`에 붙여넣으면 아무 폴더에서나 `idfenv` 한 단어로 `setup_env.ps1` 실행 가능해지는 함수 정의 | PowerShell (참고용 스니펫, 직접 실행 안 함) |
| `spec_conformance.py` | `main/*.c`가 참조하는 `CH422G_EXIO_*`/`*_GPIO_NUM` 매크로가 실제로 `pin_config.h`에 정의돼 있는지 대조 — 지어낸 매크로 이름을 컴파일 전에 잡아낸다. `verify.ps1`(빌드 게이트) 1단계로 자동 실행됨 | Python 3 (표준 라이브러리만) |
| `gen_glow_image.py` | 게이지 뒤에 까는 정적 글로우 배경(64×64 RGB565)을 생성해서 `main/ui/ui_glow_*.c`로 구움 | Python 3 + `Pillow`(`pip install Pillow`) |
| `extract_korean_chars.py` | (레거시) 한글 서브셋 폰트를 만들려고 `ui.c`에서 실제 쓰인 한글 글자만 추출하던 도구. 한글 UI 시도 자체가 실기기에서 안 돼서 영문으로 전환하며 폐기된 접근이라(`docs/design/ui-layout.md` "UI 표시 언어" 항목 참고) 지금은 안 쓰지만, 나중에 한글을 다시 시도할 일이 있으면 참고용으로 남겨둠 | Python 3 |
| `can_sim_kvaser.py` | KVASER 실물 어댑터로 `docs/hardware/vehicle.dbc` 기준 InvMsg1/InvMsg2를 실제 구동 사이클(정차→가속→순항→감속→후진)처럼 값이 계속 변하게 전송하는 벤치 테스트 시뮬레이터. CANKing의 Generator는 고정값 반복 전송만 되고 시간에 따라 값이 변하는 시뮬레이션은 못 해서 만듦(2026-07-31) | Python 3 + `pip install cantools python-can` + Kvaser 드라이버(`canlib32.dll`, CANKing 설치 시 같이 깔림) |

`main/`(펌웨어)이나 `docs/`(설계 문서)와 달리 이 폴더는 빌드 게이트(`verify.ps1`)
대상이 아니고, CLAUDE.md의 "코드 생성 전 체크리스트"도 이 폴더 자체에는 적용 안 됨
(다만 `spec_conformance.py`처럼 `main/`을 검사하는 도구는 그 검사 대상 코드의
소스 오브 트루스 규칙을 그대로 따라야 함).
