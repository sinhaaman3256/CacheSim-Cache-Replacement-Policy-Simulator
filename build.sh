#!/bin/bash

# CacheSim Build Script for Emscripten
# This script compiles the C++ cache simulator to WebAssembly

echo "Building CacheSim with Emscripten..."

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "Error: emcc (Emscripten) not found in PATH"
    echo "Please install Emscripten first: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Create wasm directory if it doesn't exist
mkdir -p wasm

# Build the WebAssembly module
echo "Compiling C++ to WebAssembly..."
em++ -std=c++17 -O3 \
    -s WASM=1 \
    -s MODULARIZE=1 \
    -s EXPORT_ES6=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXPORTED_FUNCTIONS='["_run_simulation_json","_free_json"]' \
    -s EXPORTED_RUNTIME_METHODS='["cwrap","UTF8ToString","lengthBytesUTF8","stringToUTF8","_malloc","_free"]' \
    -s NO_EXIT_RUNTIME=1 \
    -s ASSERTIONS=0 \
    -s SAFE_HEAP=0 \
    -s STACK_OVERFLOW_CHECK=0 \
    -o wasm/cachesim.js \
    core/src/*.cpp \
    wasm/bridge.cpp

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo "Generated files:"
    echo "  - wasm/cachesim.js"
    echo "  - wasm/cachesim.wasm"
    echo ""
    echo "You can now serve the web/ directory with any static server:"
    echo "  python -m http.server 8000"
    echo "  npx serve web"
    echo "  # or open web/index.html directly in a browser"
else
    echo "❌ Build failed!"
    exit 1
fi
