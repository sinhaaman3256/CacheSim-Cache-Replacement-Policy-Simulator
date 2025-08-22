@echo off
echo Building CacheSim with Emscripten...
emcc --version >nul 2>nul
if errorlevel 1 goto :error
if not exist "wasm" mkdir wasm
echo Compiling C++ to WebAssembly...
em++ -std=c++17 -O3 -s WASM=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='["_run_simulation_json","_free_json"]' -s EXPORTED_RUNTIME_METHODS='["cwrap","UTF8ToString","lengthBytesUTF8","stringToUTF8","_malloc","_free"]' -s NO_EXIT_RUNTIME=1 -s ASSERTIONS=0 -s SAFE_HEAP=0 -s STACK_OVERFLOW_CHECK=0 -o wasm/cachesim.js core/src/*.cpp wasm/bridge.cpp
if errorlevel 0 goto :success
goto :error
:success
echo Build successful!
echo Generated files: wasm/cachesim.js and wasm/cachesim.wasm
goto :end
:error
echo Build failed!
:end
pause
