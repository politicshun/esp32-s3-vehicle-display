---
description: ESP-IDF 개발 환경을 활성화하고, 현재 sdkconfig의 실제 설정값을 sdkconfig.defaults로 고정한다.
---

다음을 순서대로 실행해라.

1. `scripts\setup_env.ps1`을 **dot-sourcing으로** 실행해라: `. .\scripts\setup_env.ps1`
   (반드시 맨 앞에 점(.)과 공백을 붙일 것 — `powershell -File ...`로 새 프로세스를 띄우면
   환경변수가 그 자식 프로세스에만 설정되고 끝나자마자 사라진다. 지금 셸에 남게 하려면 dot-sourcing 필수.)
   실패하면 실제 ESP-IDF 설치 경로를 사용자에게 물어서 스크립트의 `$IdfInstallPath` 값을 고친다.

2. 이미 `sdkconfig` 파일이 프로젝트에 있다면, `idf.py save-defconfig`를 실행한다.
   이 명령은 현재 sdkconfig에서 기본값과 다른 항목만 뽑아 `sdkconfig.defaults`를 재생성한다.

3. 방금 생성된 `sdkconfig.defaults`와, 저장소에 이미 있던 `sdkconfig.defaults`(Bluetooth/NimBLE,
   flash size 항목이 미리 채워져 있음)를 비교한다.
   - 이미 있던 항목과 값이 같으면 그대로 둔다.
   - 새로 추가된 항목이 있으면(특히 legacy driver 관련, deprecation suppression 관련 CONFIG_ 항목),
     "HARNESS-TODO" 주석을 지우고 그 항목들을 채워 넣는다.
   - 두 파일 내용이 충돌하면(같은 키에 다른 값) 절대 임의로 고르지 말고 사용자에게 어느 쪽이 맞는지 물어본다.

4. 최종 `sdkconfig.defaults` 내용을 사용자에게 보여주고, `docs/design/toolchain-versions.md`에
   "sdkconfig.defaults 최신화 완료 (idf.py save-defconfig 기준, 날짜)" 한 줄을 추가한다.

5. `verify.ps1`을 실행해서 이 설정 그대로 빌드가 되는지 확인한다.

각 단계 결과를 실행하면서 요약해서 보고해라. 실패하는 단계가 있으면 다음 단계로 넘어가지 말고 원인을 먼저 진단해라.
