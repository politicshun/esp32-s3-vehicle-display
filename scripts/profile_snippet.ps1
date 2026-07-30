# $PROFILE에 아래 내용을 추가하면, 어느 폴더에서든 "idfenv" 한 단어로 환경 활성화 가능.
#
# 적용 방법 (PowerShell에서 실행):
#   notepad $PROFILE
#   (파일이 없다고 뜨면: New-Item -ItemType File -Path $PROFILE -Force)
#   아래 내용을 파일 끝에 붙여넣고 저장
#
# 그다음부터는 새 터미널에서:
#   idfenv
# 한 줄이면 환경 활성화 끝.

function idfenv {
    $projectRoot = "C:\esp_workspace\esp32_s3_vehicle_display"
    & "$projectRoot\scripts\setup_env.ps1"
}
