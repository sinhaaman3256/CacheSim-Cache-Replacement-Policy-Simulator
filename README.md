# CacheSim — Cache Replacement Policy Simulator

A fully functional client-side, zero-backend simulator that visualizes and compares cache replacement policies (LRU, LFU, ARC, FIFO) on user-provided request traces. Built with modern C++17, compiled to WebAssembly via Emscripten, and rendered with a minimal HTML/JS UI.

## ✅ Current Status

**FULLY WORKING** - WASM implementation is now fully functional with proper trace parsing, cache simulation, and real-time statistics. The project has been thoroughly debugged and tested.

## Features

- **Four Cache Policies**: LRU, LFU, ARC, and FIFO implementations
- **Two Simulation Modes**: 
  - **Animate Mode**: Step-by-step visualization with smooth animations
  - **Fast Mode**: Quick execution for large traces with sparse snapshots
- **Comparison Mode**: Run multiple policies side-by-side on the same trace
- **Interactive UI**: Play/pause/step controls with adjustable speed
- **Real-time Stats**: Hit/miss ratios, eviction counts, and policy-specific metadata
- **Zero Backend**: All computation runs in WebAssembly, deployable as static files
- **Robust Trace Parsing**: Handles multi-line traces with proper operation separation

## Architecture

```
CacheSim/
├── core/                    # C++ cache library
│   ├── include/            # Common interfaces and types
│   └── src/                # Policy implementations and simulation engine
├── wasm/                   # Emscripten bridge and build outputs
├── web/                    # Minimal HTML/JS/CSS UI
└── build scripts           # Emscripten compilation
```

## Quick Start

### Prerequisites

1. **Install Emscripten** (required for building):
   ```bash
   # On Windows, download from: https://emscripten.org/docs/getting_started/downloads.html
   # On macOS/Linux:
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh  # or ./emsdk_env.bat on Windows
   ```

2. **Verify installation**:
   ```bash
   emcc --version
   ```

### Build & Run

1. **Clone and build**:
   ```bash
   git clone <your-repo>
   cd CacheSim
   
   # Navigate to wasm directory and build
   cd wasm
   
   # On Windows (PowerShell):
   C:\Users\<username>\Desktop\emsdk\upstream\emscripten\em++.bat -std=c++17 -O2 -s WASM=1 -s EXPORTED_FUNCTIONS='["_malloc","_free","_run_simulation_json","_free_json"]' -s EXPORTED_RUNTIME_METHODS='["stringToUTF8","UTF8ToString"]' -s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=32MB -I../core/include ../core/src/simulator.cpp ../core/src/trace_parser.cpp bridge.cpp -o ../web/cachesim.js
   
   # On macOS/Linux:
   em++ -std=c++17 -O2 -s WASM=1 -s EXPORTED_FUNCTIONS='["_malloc","_free","_run_simulation_json","_free_json"]' -s EXPORTED_RUNTIME_METHODS='["stringToUTF8","UTF8ToString"]' -s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=32MB -I../core/include ../core/src/simulator.cpp ../core/src/trace_parser.cpp bridge.cpp -o ../web/cachesim.js
   ```

2. **Serve the web interface**:
   ```bash
   cd ../web
   python -m http.server 8000
   ```

3. **Open in browser**: Navigate to `http://localhost:8000`

## Usage

### Basic Operation

1. **Set cache capacity** (default: 3)
2. **Select policies** (check LRU, LFU, ARC, or FIFO)
3. **Choose mode**: Animate (step-by-step) or Fast (quick execution)
4. **Enter trace** in the textarea (default trace provided):
   ```
   PUT A apple
   PUT B banana
   PUT C cherry
   GET A
   PUT D dragon
   GET B
   PUT E eagle
   GET C
   ```
5. **Click "Run Simulation (WASM)"**

### Trace Grammar

Each line represents one operation:
- `PUT <key> <value>` - Store key-value pair
- `GET <key>` - Retrieve value for key
- `# comment` - Lines starting with # are ignored
- Empty lines are skipped

**Example traces**:
```
# Simple sequence
PUT A apple
PUT B banana
GET A
PUT C cherry
GET B
GET C

# Access pattern simulation
PUT user1 "John Doe"
PUT user2 "Jane Smith"
GET user1
GET user2
PUT user3 "Bob Johnson"
GET user1
GET user3
```

### UI Controls

- **Player Bar** (Animate mode only):
  - ⏮ Step Back: Previous step
  - ⏯ Play/Pause: Auto-advance through steps
  - ⏭ Step Forward: Next step
  - Speed slider: 0.25× to 2× playback speed
  - Step counter: Current position

- **Visualization**:
  - Cache boxes show current state
  - Blue outline: Hit on current operation
  - Yellow highlight: Miss on current operation
  - Red flash: Eviction (brief animation)
  - Frequency badges (LFU): Small numbers showing access count
  - ARC metadata: T1/T2/B1/B2 sets and adaptive parameter p

## Cache Policies

### LRU (Least Recently Used)
- **Principle**: Evict least recently accessed items
- **Behavior**: GET operations move items to front (MRU position)
- **Best for**: Temporal locality patterns

### LFU (Least Frequently Used)
- **Principle**: Evict least frequently accessed items
- **Behavior**: Tracks access frequency, breaks ties by recency
- **Best for**: Frequency-based access patterns

### ARC (Adaptive Replacement Cache)
- **Principle**: Balances recency and frequency adaptively
- **Behavior**: Maintains ghost lists (B1, B2) and adapts parameter p
- **Best for**: Mixed access patterns, self-tuning

### FIFO (First In, First Out)
- **Principle**: Evict oldest items regardless of access pattern
- **Behavior**: GET operations don't affect eviction order
- **Best for**: Simple, predictable behavior

## Performance Considerations

- **Animate Mode**: Automatically switches to Fast mode for traces >20k operations
- **Memory Usage**: Efficient C++ implementation with minimal object copying
- **WASM Size**: Optimized build with -O2, ~200-300KB total
- **Large Traces**: Fast mode handles 100k+ operations efficiently
- **WASM Performance**: C++ backend provides near-native performance

## Development

### Project Structure

```
core/
├── include/types.hpp          # Common data structures
├── src/lru_policy.hpp         # LRU implementation
├── src/fifo_policy.hpp        # FIFO implementation
├── src/lfu_policy.hpp         # LFU implementation
├── src/arc_policy.hpp         # ARC implementation
├── src/simulator.hpp          # Simulation engine
├── src/simulator.cpp
├── src/trace_parser.hpp       # Trace parsing
└── src/trace_parser.cpp

wasm/
├── bridge.cpp                 # Emscripten bindings and JSON parsing
├── cachesim.js               # Generated JS wrapper
└── cachesim.wasm             # Generated WASM binary

web/
├── index.html                 # Main UI
├── styles.css                 # Styling
├── main.js                   # UI logic and WASM integration
└── cache-simulator.js        # JavaScript fallback implementation
```

### Building

The build process:
1. Compiles C++17 code with Emscripten
2. Links all policy implementations
3. Generates JavaScript wrapper and WASM binary
4. Optimizes for size and performance
5. Exports required functions: `_malloc`, `_free`, `_run_simulation_json`, `_free_json`
6. Exports runtime methods: `stringToUTF8`, `UTF8ToString`

### Adding New Policies

1. Create new policy class implementing `IPolicy` interface
2. Add to `createPolicy()` function in `wasm/bridge.cpp`
3. Implement `metaForUI()` if policy-specific metadata is needed
4. Rebuild with the em++ command above

## Deployment

### Static Hosting

Deploy to any static hosting service:
- **GitHub Pages**: Push `/web` + `/wasm` to gh-pages branch
- **Netlify**: Drag and drop `/web` folder
- **Vercel**: Connect repository, set build output to `/web`
- **AWS S3 + CloudFront**: Upload static files

### Requirements

- Modern browser with WebAssembly support
- HTTPS recommended (WASM loading restrictions)
- No server-side processing required

## Testing

### Example Traces

**Basic functionality** (cache capacity 2):
```
PUT A apple
PUT B banana
GET A
PUT C cherry
GET B
GET C
```

**Extended testing** (cache capacity 3):
```
PUT A apple
PUT B banana
PUT C cherry
GET A
PUT D dragon
GET B
PUT E eagle
GET C
```

**Eviction testing**:
```
PUT A 1
PUT B 2
PUT C 3
PUT D 4  # Should evict A (LRU) or A (FIFO)
GET A    # Should miss
```

**Frequency patterns**:
```
PUT A 1
PUT B 2
GET A
GET A
GET A
PUT C 3
GET B
GET B
PUT D 4  # Should evict C (LFU) or A (LRU)
```

## Troubleshooting

### Common Issues

1. **"WASM module not loaded"**: Check browser console for errors, ensure HTTPS
2. **Build failures**: Verify Emscripten installation, check C++ syntax
3. **Performance issues**: Use Fast mode for large traces, check browser dev tools
4. **UI not responding**: Check JavaScript console for errors
5. **Trace parsing issues**: Ensure each operation is on a separate line

### Browser Support

- **Chrome**: 57+ (full support)
- **Firefox**: 52+ (full support)
- **Safari**: 11+ (full support)
- **Edge**: 79+ (full support)

### Debug Information

The WASM module includes comprehensive debug output:
- JSON parsing details
- Trace text extraction
- Line-by-line operation parsing
- Cache simulation steps

Check the browser console for detailed debug information when running simulations.

## Recent Fixes & Improvements

- ✅ **Fixed WASM loading**: Proper module initialization and function exports
- ✅ **Fixed trace parsing**: Robust handling of multi-line traces with escaped newlines
- ✅ **Fixed JSON parsing**: Correct extraction of trace text from JSON requests
- ✅ **Added debug output**: Comprehensive logging for troubleshooting
- ✅ **Cleaned up code**: Removed unnecessary files and debug code
- ✅ **Enhanced test cases**: Bigger default trace with cache capacity 3

