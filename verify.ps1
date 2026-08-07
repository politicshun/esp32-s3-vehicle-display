# verify.ps1 — 빌드 게이트 (AGENTS.md 4번 항목)
# Case B(ESP-IDF 구조체 필드 삭제), Case C(컴포넌트 미설치)는
# spec_conformance.py로는 못 잡는다 — 실제 idf.py build만이 확정적으로 잡아낸다.
# 그래서 이 두 케이스는 "생성 전에 100% 확정 가능한 사실"이 아니라
# "생성 후 빌드해봐야 확정되는 사실"로 분류하고, 이 게이트가 그 역할을 전담한다.

Write-Host "=== 1/2: spec-conformance (매크로 이름 대조) ===" -ForegroundColor Cyan
python scripts\spec_conformance.py main\include\pin_config.h main\init.c main\twai.c main\lvgl.c main\ble.c
if ($LASTEXITCODE -ne 0) {
    Write-Host "spec-conformance 실패 - 빌드 진행 안 함" -ForegroundColor Red
    exit 1
}

Write-Host "=== 2/2: idf.py build (실제 툴체인/컴포넌트 검증) ===" -ForegroundColor Cyan
# 2026-08-07: idf.py가 PATH에 없으면 PowerShell은 CommandNotFoundException을 던지는데,
# 이건 네이티브 프로세스 종료가 아니라서 $LASTEXITCODE를 갱신하지 않는다. 그러면 바로 위
# python(spec_conformance)의 0이 그대로 남아 아래 검사를 통과해버리고, 빌드를 한 번도
# 하지 않은 채 "전체 게이트 통과"가 찍힌다 — 실제로 발생했음. 명시적으로 먼저 막는다.
if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    Write-Host "idf.py를 찾을 수 없음 - ESP-IDF 환경이 활성화되지 않았습니다." -ForegroundColor Red
    Write-Host "지금 쓰는 세션 '안에서' 다음을 먼저 실행할 것:" -ForegroundColor Yellow
    Write-Host "    & .\scripts\setup_env.ps1" -ForegroundColor Yellow
    Write-Host "(powershell -File 로 새 프로세스를 띄우면 IDF_PATH가 같이 사라집니다)" -ForegroundColor Yellow
    exit 1
}

idf.py build
if ($LASTEXITCODE -ne 0) {
    Write-Host "빌드 실패 - 에러 로그를 AI에게 그대로 전달할 것 (검색 기반 재확정 필요)" -ForegroundColor Red
    exit 1
}

Write-Host "✅ 전체 게이트 통과" -ForegroundColor Green
