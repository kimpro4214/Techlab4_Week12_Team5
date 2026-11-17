@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

rem 기본 Root 및 Bin, pdb, exe 파일명 세팅
set "RooT=%~dp0"
set "ROOTNS=%ROOT:~0,-1%"
set "TARGET=HitoriEditor"
set "BIN=%ROOT%Build\Bin\Release-windows-x86_64"
set "PDB=%BIN%\%TARGET%.pdb"
set "EXE=%BIN%\%TARGET%.exe"

rem 사용할 Debug Tool 경로, symbol, source 저장한 로컬경로와 공유폴더에서의 UNC 경로 세팅
set "DBGTOOLS=C:\Program Files (x86)\Windows Kits\10\Debuggers\x64"
set "SYM_LOCAL=C:\SymTest\Symbols"
set "SRC_LOCAL=C:\SymTest\Source"
set "SRC_UNC=\\172.21.10.125\source"

rem 소스 파일 경로 저장할 임시 txt와 해당 txt 바탕으로 한 스트림 파일 저장할 경로 세팅
set "WORK=%ROOT%Build\SymbolPublish"
set "LIST=%WORK%\files.txt"
set "SRCSRV=%WORK%\srcsrv.txt"
if not exist "%WORK%" mkdir "%WORK%"

if not exist "%PDB%" (
    echo [Publish] %PDB% not found. Build Release first.
    exit /b 1
)
if not exist "%EXE%" (
    echo [Publish] %EXE% not found. Build Release first.
    exit /b 1
)

rem 버전 = 커밋 해시 + exe 빌드 시각 (같은 빌드는 같은 이름, 다시 빌드하면 다른 이름)
rem PDB는 pdbstr가 고쳐 써서 수정 시각이 바뀌므로, 스크립트가 읽기만 하는 exe의 수정 시각을 사용
for /f %%H in ('git -C "%ROOTNS%" rev-parse --short HEAD') do set "VERSION=%%H"
for /f "usebackq" %%T in (`powershell -NoProfile -Command "(Get-Item -LiteralPath '%EXE%').LastWriteTime.ToString('yyMMdd_HHmmss')"`) do set "STAMP=%%T"
if not defined STAMP (
    echo [Publish] Failed to read build time of %EXE%.
    exit /b 1
)
set "VERSION=%VERSION%_%STAMP%"
echo [Publish] VERSION=%VERSION%

rem pdb가 기억하는 소스 파일 절대경로 전부 출력하고 SDK,MSVC는 뺀 소스만 file.txt에 저장, srctool.exe 이용
"%DBGTOOLS%\srcsrv\srctool.exe" -r "%PDB%" | findstr /l /b /i /c:"%ROOTNS%" | findstr /v /c:"source files are indexed" > "%LIST%"

rem Source 파일들 Version 정보 붙여서 Symbol Server Local 폴더에 저장함 (소스 스냅샷)
robocopy "%ROOT%Source" "%SRC_LOCAL%\%VERSION%\Source" *.h *.hpp *.cpp *.c *.inl /e /njh /njs /nfl /ndl
if errorlevel 8 exit /b 1
robocopy "%ROOT%Intermediate\Generated" "%SRC_LOCAL%\%VERSION%\Intermediate\Generated" *.h /e /njh /njs /nfl /ndl
if errorlevel 8 exit /b 1

rem 스냅샷을 통한 srcsrv.txt 스트림 파일 작성
rem SRCSRVTRG = srcsrv의 Target, 디버거가 소스 파일을 찾는 최종 위치
rem 공유폴더로 세팅했으므로 Local 경로가 아닌 공유폴더의 UNC 경로로 표시
> "%SRCSRV%" echo SRCSRV: ini ------------------------------------------------
>>"%SRCSRV%" echo VERSION=2
>>"%SRCSRV%" echo SRCSRV: variables ------------------------------------------
>>"%SRCSRV%" echo SRCSRVTRG=%SRC_UNC%\%VERSION%\%%var2%%
>>"%SRCSRV%" echo SRCSRV: source files ---------------------------------------

for /f "usebackq delims=" %%L in ("%LIST%") do (
    set "ABS=%%L"
    set "REL=!ABS:%ROOT%=!"
    >>"%SRCSRV%" echo !ABS!*!REL!
)

>>"%SRCSRV%" echo SRCSRV: end ------------------------------------------------

rem pdbstr.exe를 통해 srcsrv를 pdb 파일에 기록함
"%DBGTOOLS%\srcsrv\pdbstr.exe" -w -p:"%PDB%" -i:"%SRCSRV%" -s:srcsrv

"%DBGTOOLS%\srcsrv\pdbstr.exe" -r -p:"%PDB%" -s:srcsrv | findstr /c:"SRCSRVTRG" >nul
if errorlevel 1 (
    echo [Publish] srcsrv stream was not written.
    exit /b 1
)

rem 최종 기록이 완료된 pdb와 exe를 SymbolServer에 등록, sysmstore.exe 이용
"%DBGTOOLS%\symstore.exe" add /f "%PDB%" /s "%SYM_LOCAL%" /t Hitori /v %VERSION%
"%DBGTOOLS%\symstore.exe" add /f "%EXE%" /s "%SYM_LOCAL%" /t Hitori /v %VERSION%

echo [Publish] Done: %VERSION%
endlocal
