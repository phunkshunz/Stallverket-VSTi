@echo off
rem Installerar STALLVERKET på Windows. Dubbelklicka på filen.
rem Plugin-mappen för VST3 kräver administratör, så filen ber om det själv.

net session >nul 2>&1
if errorlevel 1 (
  powershell -NoProfile -Command "Start-Process -Verb RunAs -FilePath '%~f0'"
  exit /b
)

cd /d "%~dp0"
set "DEST=%CommonProgramFiles%\VST3"
if not exist "%DEST%" mkdir "%DEST%"
if exist "%DEST%\STALLVERKET.vst3" rmdir /s /q "%DEST%\STALLVERKET.vst3"
xcopy "STALLVERKET.vst3" "%DEST%\STALLVERKET.vst3\" /e /i /y /q >nul

if errorlevel 1 (
  echo.
  echo Nagot gick fel. Packade du upp zip-filen forst? Hogerklicka pa zip-filen och valj "Extrahera alla".
  pause
  exit /b 1
)

rem Ljudbanken (BETONGs ljud) for BETONG-KASSETTEN, delas av alla MASKINHALLEN-maskiner
if exist "Ljudbank" (
  if not exist "%USERPROFILE%\Music\MASKINHALLEN\Ljudbank" mkdir "%USERPROFILE%\Music\MASKINHALLEN\Ljudbank"
  xcopy "Ljudbank" "%USERPROFILE%\Music\MASKINHALLEN\Ljudbank\" /e /i /y /q >nul
  echo Ljudbanken ligger i %USERPROFILE%\Music\MASKINHALLEN\Ljudbank
)

echo.
echo Klart! STALLVERKET ligger nu i:
echo   %DEST%
echo Starta om din DAW, eller kor en ny plugin-sokning.
echo.
pause
