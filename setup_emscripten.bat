@echo off
echo Setting up Emscripten environment permanently...

REM Use the correct absolute path for EMSDK_DIR
set "EMSDK_DIR=C:\Users\sinha\OneDrive\Desktop\emsdk"

REM Check if emsdk directory exists
if not exist "%EMSDK_DIR%" (
    echo Error: emsdk directory not found at %EMSDK_DIR%
    echo Please install Emscripten first
    pause
    exit /b 1
)

REM Set environment variables permanently
setx EMSDK "%EMSDK_DIR%"
setx EMSDK_NODE "%EMSDK_DIR%\node\22.16.0_64bit\bin\node.exe"
setx EMSDK_PYTHON "%EMSDK_DIR%\python\3.13.3_64bit\python.exe"

REM Add to PATH permanently
setx PATH "%PATH%;%EMSDK_DIR%;%EMSDK_DIR%\upstream\emscripten"

echo.
echo Emscripten environment variables set permanently!
echo.
echo You can now use emcc and em++ from any command prompt.
echo.
echo To test, open a new command prompt and run:
echo   emcc --version
echo.
pause
