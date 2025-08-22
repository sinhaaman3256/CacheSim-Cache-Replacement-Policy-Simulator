# CacheSim — Cache Replacement Policy Simulator

**CacheSim** is a browser-based app that lets you **visualize and compare cache replacement policies** in real time.  
The simulation engine is written in **C++17** and compiled to **WebAssembly (WASM)** for near-native speed, while the UI is built with **React** for an interactive, student-friendly experience.

🌐 **Live Demo**: [CacheSim on Vercel](https://cachesim-707s0hfs4-aman-sinhas-projects-9781eb36.vercel.app/)

This project is focused on **learning**: if you’re studying operating systems, databases, or computer architecture, CacheSim helps you build intuition about **hits, misses, evictions, and policy trade-offs**.

---

## Highlights

- **Four Policies**: FIFO, LRU, LFU, ARC  
- **Two Modes**:
  - **Animate** — step-by-step playback with controls
  - **Fast** — high-speed execution with snapshots for large traces  
- **Side-by-Side Comparison** — run multiple policies on the same trace and compare stats  
- **Real-Time Stats** — hits, misses, hit ratio, evictions  
- **Policy Metadata** — LFU frequency counters; ARC’s T1/T2/B1/B2 with adaptive `p`  
- **Zero Backend** — runs fully in the browser via WASM

---

## UI Controls

- **Animate mode**: Step Back, Play/Pause, Step Forward, speed control, step counter  
- **Visualization**:
  - Cache boxes show current state
  - **Hit** = highlighted (e.g., blue)
  - **Miss** = highlighted (e.g., yellow)
  - **Eviction** = red flash
  - LFU shows frequency badges; ARC shows T1/T2/B1/B2 and `p`

---

## What You’ll Learn (Policies in Plain English)

- **LRU (Least Recently Used)**  
  Evicts the item that hasn't been accessed for the longest time — good for temporal locality.

- **LFU (Least Frequently Used)**  
  Evicts the item with the lowest access count — captures long-term popularity but adapts slowly.

- **ARC (Adaptive Replacement Cache)**  
  Balances recency and frequency with two resident lists (T1, T2) and two ghost lists (B1, B2). ARC adapts an internal parameter `p` to shift capacity toward recency or frequency as workload changes.

- **FIFO (First-In, First-Out)**  
  Evicts the oldest inserted item, regardless of access frequency or recency. Simple baseline behavior.

---

## Why WebAssembly?

- **Performance**: simulation performs many mutations and lookups; C++ → WASM keeps the UI responsive for large traces.  
- **Portable**: runs entirely in the browser — no server setup required for users.  
- **Separation of concerns**: C++/WASM for the simulation engine, React for visualization & UX.

---

## Example Scenarios

**Textual request trace** (capacity = 3):

```text
PUT A apple
PUT B banana
PUT C cherry
GET A
PUT D dragon
GET B
PUT E eagle
GET C
```

- **LRU**: `A` is promoted on `GET A` and less likely to be evicted soon.  
- **LFU**: frequency badges show which keys are accessed most and therefore kept.  
- **FIFO**: evicts by insertion order — oldest inserted leaves first.  
- **ARC**: T1/T2/B1/B2 and `p` will shift as the workload patterns change.

**Numeric reference string** (capacity = 3):

```text
1 2 3 1 4 2 5
```

Interpretation:
- Hits are highlighted immediately.
- Misses show insertion + which entry is evicted.
- LFU counters and ARC partitions will update visibly.

---

## Build & Run (Local)

**Prereqs**
- Node.js + npm
- (Optional) Emscripten — only if you want to rebuild the WASM from C++

### 1) Clone and install frontend

```bash
git clone https://github.com/sinhaaman3256/CacheSim-Cache-Replacement-Policy-Simulator.git
cd CacheSim-Cache-Replacement-Policy-Simulator/web
npm install
```

### 3) Start dev server

```bash
npm start
```

Open `http://localhost:3000/`

---

## Rebuilding the WebAssembly Module (Optional)

If you change the C++ core or add policies, rebuild to regenerate `cachesim.js` + `cachesim.wasm`.

### Prepare Emscripten environment

- **PowerShell users** (Windows):

```powershell
# From the emsdk folder (or full path)
& "C:\path\to\emsdk\emsdk_env.ps1"
# If blocked by policy:
# Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

- **CMD users**:

```cmd
C:\path\to\emsdk\emsdk_env.bat
```

### Build command

Run from the repository root (so paths are relative):

**Windows / PowerShell** (use the backtick ` for line continuation):

```powershell
em++ -std=c++17 -O2 `
  -s WASM=1 `
  -s EXPORTED_FUNCTIONS='["_malloc","_free","_run_simulation_json","_free_json"]' `
  -s EXPORTED_RUNTIME_METHODS='["stringToUTF8","UTF8ToString"]' `
  -s ALLOW_MEMORY_GROWTH=1 `
  -s MAXIMUM_MEMORY=32MB `
  -Icore/include `
  core/src/simulator.cpp core/src/trace_parser.cpp wasm/bridge.cpp `
  -o web/public/cachesim.js
```

**macOS / Linux** (single-line or backslash continuation):

```bash
em++ -std=c++17 -O2 \
  -s WASM=1 \
  -s EXPORTED_FUNCTIONS='["_malloc","_free","_run_simulation_json","_free_json"]' \
  -s EXPORTED_RUNTIME_METHODS='["stringToUTF8","UTF8ToString"]' \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MAXIMUM_MEMORY=32MB \
  -Icore/include \
  core/src/simulator.cpp core/src/trace_parser.cpp wasm/bridge.cpp \
  -o web/public/cachesim.js
```

### Exports expected by the UI

- `_run_simulation_json` — accepts a JSON request pointer and returns a JSON result pointer  
- `_free_json` — frees the returned JSON string  
- `_malloc`, `_free` — used by the JS glue for transferring strings  
- Runtime methods: `stringToUTF8`, `UTF8ToString`

---

## Architecture (at a glance)

- **core/** — C++ cache library (interfaces, policies, simulator engine, trace parser)  
- **wasm/** — `bridge.cpp` — Emscripten glue that exposes a JSON API to JS  
- **web/** — React app (Create React App)  
  - **web/public/** — static assets served as-is (WASM + glue JS + HTML + CSS + `main.js`)  
  - **web/src/** — React components and app logic

---

## Designed for Learning

- Compare algorithms under identical inputs to see **why** one policy wins.  
- Visualize **temporal locality**, **frequency bias**, and **adaptive behavior**.  
- Ideal for assignments, demos, and self-study.

---

## Future Ideas

- Add more policies: Random, CLOCK, 2Q, Belady’s (offline optimal)  
- CSV/JSON trace import/export and preset workloads (Zipfian, scans, bursts)  
- Time-series charts (miss ratio over time) and exportable traces

---
