@echo off
Title Install Application Startup RegKey

set APP_NAME=InternetCheck.exe
set APP_PATH=%~dp0%APP_NAME%

echo Adding %APP_NAME% to startup...
reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v %APP_NAME% /t REG_SZ /d "%APP_PATH%" /f

if %ERRORLEVEL% == 0 (
    echo Successfully added %APP_NAME% to startup.
) else (
    echo Failed to add %APP_NAME% to startup.
)

pause
