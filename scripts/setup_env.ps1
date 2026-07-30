# scripts/setup_env.ps1
# ESP-IDF 환경변수(IDF_PATH, PATH, Python venv)는 프로세스 단위라 터미널을 새로 켤 때마다 사라진다.
# 이 스크립트가 그걸 매번 대신 해준다.
#
# !! 실행 방법 주의 !!
# 반드시 지금 쓰고 있는 PowerShell 세션 "안에서" 실행해야 한다:
#   & .\scripts\setup_env.ps1     또는     . .\scripts\setup_env.ps1
# "powershell -File scripts\setup_env.ps1"처럼 새 프로세스를 띄워서 실행하면,
# 환경변수가 그 새 프로세스 안에서만 설정되고 끝나자마자 사라져서
# 원래 세션에서는 idf.py가 계속 "not recognized"로 남는다 (실제로 한 번 발생했음).

$IdfInstallPath = "C:\esp\v6.0.2\esp-idf"   # HARNESS-TODO: 실제 설치 경로와 다르면 이 줄만 수정

if (-not (Test-Path "$IdfInstallPath\export.ps1")) {
    Write-Host "❌ export.ps1을 못 찾았습니다: $IdfInstallPath" -ForegroundColor Red
    Write-Host "   실제 ESP-IDF 설치 경로를 확인해서 이 스크립트 상단의 `$IdfInstallPath 값을 고쳐주세요." -ForegroundColor Yellow
    exit 1
}

Write-Host "ESP-IDF 환경 활성화 중... ($IdfInstallPath)" -ForegroundColor Cyan
& "$IdfInstallPath\export.ps1"

if ($env:IDF_PATH) {
    Write-Host "✅ 환경 활성화 완료 (IDF_PATH=$env:IDF_PATH)" -ForegroundColor Green
} else {
    Write-Host "⚠️ IDF_PATH가 안 잡혔습니다 - export.ps1 출력 로그를 확인하세요" -ForegroundColor Yellow
}
