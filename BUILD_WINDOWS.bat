@echo off
setlocal
cd /d "%~dp0"
echo.
echo ===========================================
echo   NOIRNUIT REVERSE - Compilation VST3 x64
echo ===========================================
echo.
echo Prerequis : Visual Studio 2022 (C++), CMake, Git, connexion Internet.
echo.
where cmake >nul 2>nul || (echo ERREUR : CMake absent. & pause & exit /b 1)
where git >nul 2>nul || (echo ERREUR : Git absent. & pause & exit /b 1)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
if errorlevel 1 (echo ERREUR : Configuration CMake. & pause & exit /b 1)
cmake --build build --config Release --target NoirnuitReverse_VST3 noirnuit-render reverse-engine-tests --parallel 4
if errorlevel 1 (echo ERREUR : Compilation. & pause & exit /b 1)
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 (echo ATTENTION : Les tests DSP ont echoue. & pause & exit /b 1)
echo.
echo Recherche du VST3 compile...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$p=Get-ChildItem -Path '.\build' -Directory -Filter '*.vst3' -Recurse | Select-Object -First 1; if ($null -eq $p) { Write-Host 'VST3 introuvable'; exit 1 }; $dest=Join-Path $env:CommonProgramFiles 'VST3'; New-Item -ItemType Directory -Force -Path $dest | Out-Null; Copy-Item -LiteralPath $p.FullName -Destination $dest -Recurse -Force; Write-Host ('Plugin copie vers '+$dest)"
if errorlevel 1 (echo Copie automatique echouee. Copiez le dossier *.vst3 du build dans C:\Program Files\Common Files\VST3. & pause & exit /b 1)
echo.
echo TERMINE ! Ouvrez FL Studio ^> Manage plugins ^> Find installed plugins.
pause
