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
idf.py build
if ($LASTEXITCODE -ne 0) {
    Write-Host "빌드 실패 - 에러 로그를 AI에게 그대로 전달할 것 (검색 기반 재확정 필요)" -ForegroundColor Red
    exit 1
}

Write-Host "✅ 전체 게이트 통과" -ForegroundColor Green
