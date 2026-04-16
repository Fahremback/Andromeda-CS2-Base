@echo off
REM ============================================
REM Andromeda CS2 - Build System Rápido
REM ============================================
REM Uso: build_fast.bat [modulo] [config]
REM Exemplos:
REM   build_fast.bat                    - Build completo
REM   build_fast.bat aimbot             - Build só módulo aimbot
REM   build_fast.bat all Release        - Build tudo em Release
REM   build_fast.bat visuals Debug      - Build módulo em Debug
REM ============================================

setlocal enabledelayedexpansion

REM Configurações
set PROJECT_ROOT=%~dp0
set BUILD_DIR=%PROJECT_ROOT%build
set MODULES_DIR=%PROJECT_ROOT%modules
set CONFIG=%2
if "%CONFIG%"=="" set CONFIG=Release

REM Cores para output
set "GREEN=[32m"
set "YELLOW=[33m"
set "BLUE=[34m"
set "RESET=[0m"

echo %GREEN%============================================%RESET%
echo %GREEN%  Andromeda CS2 - Fast Build System%RESET%
echo %GREEN%============================================%RESET%
echo.

REM Verificar se é build de módulo específico
if not "%1"=="" (
    if "%1"=="all" (
        goto :BUILD_ALL
    ) else if "%1"=="clean" (
        goto :CLEAN
    ) else (
        goto :BUILD_MODULE
    )
)

:BUILD_ALL
echo %BLUE%Building all modules...%RESET%
echo.

REM Build projeto principal
echo %YELLOW%Building Andromeda Bridge...%RESET%
if exist "%PROJECT_ROOT%Andromeda-CS2-Base.sln" (
    msbuild "%PROJECT_ROOT%Andromeda-CS2-Base.sln" /p:Configuration=%CONFIG% /v:q /nologo
    if errorlevel 1 (
        echo %RED%Failed to build bridge!%RESET%
        exit /b 1
    )
    echo %GREEN%Bridge built successfully!%RESET%
) else (
    echo %YELLOW%Solution file not found, skipping bridge build%RESET%
)

echo.

REM Build todos os módulos na pasta modules/
if exist "%PROJECT_ROOT%modules" (
    for /d %%D in ("%PROJECT_ROOT%modules\*") do (
        if exist "%%D\CMakeLists.txt" (
            echo %YELLOW%Building module: %%~nxD%RESET%
            call :BUILD_CMAKE_MODULE "%%D"
        ) else if exist "%%D\*.vcxproj" (
            echo %YELLOW%Building module: %%~nxD%RESET%
            call :BUILD_VCXPROJ_MODULE "%%D"
        )
    )
)

echo.
echo %GREEN%============================================%RESET%
echo %GREEN%  Build Complete!%RESET%
echo %GREEN%============================================%RESET%
goto :EOF

:BUILD_MODULE
set MODULE_NAME=%1
echo %BLUE%Building module: %MODULE_NAME%%RESET%
echo.

REM Procurar módulo
set MODULE_PATH=
if exist "%PROJECT_ROOT%modules\%MODULE_NAME%" (
    set MODULE_PATH=%PROJECT_ROOT%modules\%MODULE_NAME%
) else if exist "%PROJECT_ROOT%Andromeda-Module-%MODULE_NAME%" (
    set MODULE_PATH=%PROJECT_ROOT%Andromeda-Module-%MODULE_NAME%
)

if "%MODULE_PATH%"=="" (
    echo %RED%Module '%MODULE_NAME%' not found!%RESET%
    echo Available modules:
    if exist "%PROJECT_ROOT%modules" (
        dir /b "%PROJECT_ROOT%modules"
    )
    exit /b 1
)

REM Build módulo
if exist "%MODULE_PATH%\CMakeLists.txt" (
    call :BUILD_CMAKE_MODULE "%MODULE_PATH%"
) else if exist "%MODULE_PATH%\*.vcxproj" (
    call :BUILD_VCXPROJ_MODULE "%MODULE_PATH%"
) else (
    echo %RED%No build system found in module!%RESET%
    exit /b 1
)

echo.
echo %GREEN%Module '%MODULE_NAME%' built successfully!%RESET%
echo %YELLOW%Output: %MODULES_DIR%\%MODULE_NAME%.dll%RESET%
goto :EOF

:BUILD_CMAKE_MODULE
set MODULE_PATH=%~1
set MODULE_NAME=%~nx1

echo %BLUE%CMake build for %MODULE_NAME%%RESET%

REM Criar build directory
if not exist "%MODULE_PATH%\build_%CONFIG%" mkdir "%MODULE_PATH%\build_%CONFIG%"

REM Configurar e build
cd /d "%MODULE_PATH%\build_%CONFIG%"
cmake .. -DCMAKE_BUILD_TYPE=%CONFIG% -DOUTPUT_DIR="%MODULES_DIR%" -G "Visual Studio 17 2022"
if errorlevel 1 (
    echo %RED%CMake configure failed!%RESET%
    cd /d "%PROJECT_ROOT%"
    exit /b 1
)

cmake --build . --config %CONFIG%
if errorlevel 1 (
    echo %RED%CMake build failed!%RESET%
    cd /d "%PROJECT_ROOT%"
    exit /b 1
)

cd /d "%PROJECT_ROOT%"
goto :EOF

:BUILD_VCXPROJ_MODULE
set MODULE_PATH=%~1
set MODULE_NAME=%~nx1

echo %BLUE%MSBuild for %MODULE_NAME%%RESET%

REM Encontrar .vcxproj
for %%F in ("%MODULE_PATH%\*.vcxproj") do (
    set PROJ_FILE=%%F
)

if "%PROJ_FILE%"=="" (
    echo %RED%No .vcxproj found!%RESET%
    exit /b 1
)

msbuild "%PROJ_FILE%" /p:Configuration=%CONFIG% /v:q /nologo
if errorlevel 1 (
    echo %RED%MSBuild failed!%RESET%
    exit /b 1
)

goto :EOF

:CLEAN
echo %YELLOW%Cleaning build directories...%RESET%

if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    echo Cleaned: %BUILD_DIR%
)

if exist "%MODULES_DIR%" (
    for /d %%D in ("%MODULES_DIR%\*") do (
        if exist "%%D\build_*" (
            rmdir /s /q "%%D\build_*"
            echo Cleaned: %%D\build_*
        )
    )
)

REM Manter DLLs dos módulos
echo %GREEN%Keeping module DLLs in %MODULES_DIR%%RESET%

echo.
echo %GREEN%Clean complete!%RESET%
goto :EOF

REM ============================================
REM Watch and Build Mode
REM ============================================
:WATCH_MODE
echo %BLUE%Starting watch mode...%RESET%
echo Monitoring modules directory for changes...
echo Press Ctrl+C to stop
echo.

:WATCH_LOOP
timeout /t 2 /nobreak >nul

REM Verificar mudanças (implementação simples)
REM Para produção, usar PowerShell FileSystemWatcher

goto :WATCH_LOOP
