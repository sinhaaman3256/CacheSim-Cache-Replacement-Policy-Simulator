import React, { useState, useEffect, useRef } from 'react';
import './App.css';

function App() {
  const [wasmModule, setWasmModule] = useState(null);
  const [capacity, setCapacity] = useState(3);
  const [selectedPolicies, setSelectedPolicies] = useState(['LRU']);
  const [mode, setMode] = useState('animate');
  const [snapshotEvery, setSnapshotEvery] = useState(1000);
  const [traceText, setTraceText] = useState(`PUT A apple
PUT B banana
PUT C cherry
GET A
PUT D dragon
GET B
PUT E eagle
GET C`);
  const [results, setResults] = useState(null);
  const [currentStep, setCurrentStep] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [speed, setSpeed] = useState(1.0);
  const [isLoading, setIsLoading] = useState(false);
  const playIntervalRef = useRef(null);

  const policies = [
    { id: 'LRU', name: 'LRU', icon: '⏰', description: 'Least Recently Used' },
    { id: 'FIFO', name: 'FIFO', icon: '📋', description: 'First In, First Out' },
    { id: 'LFU', name: 'LFU', icon: '📊', description: 'Least Frequently Used' },
    { id: 'ARC', name: 'ARC', icon: '⚖️', description: 'Adaptive Replacement Cache' }
  ];

  useEffect(() => {
    initializeWASM();
  }, []);

  useEffect(() => {
    if (isPlaying) {
      startPlayback();
    } else {
      stopPlayback();
    }
  }, [isPlaying, speed]);

  const initializeWASM = async () => {
    try {
      let attempts = 0;
      const maxAttempts = 50;
      
      while (attempts < maxAttempts) {
        if (window.Module && window.Module._run_simulation_json && 
            window.Module.stringToUTF8 && window.Module.UTF8ToString) {
          setWasmModule(window.Module);
          console.log('✅ WASM module loaded successfully');
          return;
        }
        await new Promise(resolve => setTimeout(resolve, 100));
        attempts++;
      }
      throw new Error('WASM module failed to load');
    } catch (error) {
      console.error('Failed to initialize WASM:', error);
      alert('Failed to load WebAssembly module. Please refresh the page.');
    }
  };

  const runSimulation = async () => {
    if (!wasmModule) return;
    
    setIsLoading(true);
    try {
      const request = {
        capacity,
        policies: selectedPolicies,
        animate: mode === 'animate',
        snapshotEvery,
        traceText: traceText.trim()
      };

      const requestJson = JSON.stringify(request);
      const requestPtr = wasmModule._malloc(requestJson.length + 1);
      wasmModule.stringToUTF8(requestJson, requestPtr, requestJson.length + 1);
      
      const resultPtr = wasmModule._run_simulation_json(requestPtr);
      const resultStr = wasmModule.UTF8ToString(resultPtr);
      
      wasmModule._free(requestPtr);
      wasmModule._free_json(resultPtr);
      
      const result = JSON.parse(resultStr);
      setResults(Array.isArray(result) ? result : [result]);
      setCurrentStep(0);
      setIsPlaying(false);
    } catch (error) {
      console.error('Simulation failed:', error);
      alert('Simulation failed: ' + error.message);
    } finally {
      setIsLoading(false);
    }
  };

  const startPlayback = () => {
    if (playIntervalRef.current) clearInterval(playIntervalRef.current);
    
    const interval = Math.max(100, 1000 / speed);
    playIntervalRef.current = setInterval(() => {
      setCurrentStep(prev => {
        if (!results || results.length === 0) return prev;
        const maxSteps = results[0].steps?.length || 0;
        if (prev >= maxSteps - 1) {
          setIsPlaying(false);
          return prev;
        }
        return prev + 1;
      });
    }, interval);
  };

  const stopPlayback = () => {
    if (playIntervalRef.current) {
      clearInterval(playIntervalRef.current);
      playIntervalRef.current = null;
    }
  };

  const stepBack = () => {
    setCurrentStep(prev => Math.max(0, prev - 1));
  };

  const stepForward = () => {
    if (!results || results.length === 0) return;
    const maxSteps = results[0].steps?.length || 0;
    setCurrentStep(prev => Math.min(maxSteps - 1, prev + 1));
  };

  const reset = () => {
    setResults(null);
    setCurrentStep(0);
    setIsPlaying(false);
  };

  const togglePolicy = (policyId) => {
    setSelectedPolicies(prev => 
      prev.includes(policyId) 
        ? prev.filter(p => p !== policyId)
        : [...prev, policyId]
    );
  };

  const getCurrentStepData = () => {
    if (!results || results.length === 0) return null;
    const result = results[0];
    const steps = result.steps || [];
    return steps[currentStep] || null;
  };

  return (
    <div className="App">
      {/* Header */}
      <header className="app-header">
        <div className="header-content">
          <h1 className="main-title">
            <span className="title-icon">🧠</span>
            CacheSim
          </h1>
          <p className="subtitle">Advanced Cache Replacement Policy Simulator</p>
        </div>
      </header>

      <div className="main-container">
        {/* Controls Panel */}
        <div className="controls-panel">
          <div className="control-section">
            <h3>Configuration</h3>
            <div className="control-grid">
              <div className="control-item">
                <label>Cache Capacity</label>
                <input
                  type="number"
                  value={capacity}
                  onChange={(e) => setCapacity(parseInt(e.target.value) || 3)}
                  min="1"
                  max="100"
                  className="control-input"
                />
              </div>
              
              <div className="control-item">
                <label>Simulation Mode</label>
                <div className="radio-group">
                  <label className="radio-option">
                    <input
                      type="radio"
                      value="animate"
                      checked={mode === 'animate'}
                      onChange={(e) => setMode(e.target.value)}
                    />
                    <span>🎬 Animate</span>
                  </label>
                  <label className="radio-option">
                    <input
                      type="radio"
                      value="fast"
                      checked={mode === 'fast'}
                      onChange={(e) => setMode(e.target.value)}
                    />
                    <span>⚡ Fast</span>
                  </label>
                </div>
              </div>

              {mode === 'fast' && (
                <div className="control-item">
                  <label>Snapshot Every</label>
                  <input
                    type="number"
                    value={snapshotEvery}
                    onChange={(e) => setSnapshotEvery(parseInt(e.target.value) || 1000)}
                    min="1"
                    className="control-input"
                  />
                </div>
              )}
            </div>
          </div>

          <div className="control-section">
            <h3>Cache Policies</h3>
            <div className="policies-grid">
              {policies.map(policy => (
                <label key={policy.id} className="policy-option">
                  <input
                    type="checkbox"
                    checked={selectedPolicies.includes(policy.id)}
                    onChange={() => togglePolicy(policy.id)}
                  />
                  <div className="policy-content">
                    <span className="policy-icon">{policy.icon}</span>
                    <div className="policy-info">
                      <span className="policy-name">{policy.name}</span>
                      <span className="policy-desc">{policy.description}</span>
                    </div>
                  </div>
                </label>
              ))}
            </div>
          </div>

          <div className="control-section">
            <h3>Trace Input</h3>
            <textarea
              value={traceText}
              onChange={(e) => setTraceText(e.target.value)}
              placeholder="Enter cache operations (e.g., PUT A apple, GET A)"
              rows="8"
              className="trace-input"
            />
          </div>

          <div className="action-buttons">
            <button
              onClick={runSimulation}
              disabled={!wasmModule || isLoading || selectedPolicies.length === 0}
              className="btn btn-primary"
            >
              {isLoading ? '🔄 Running...' : '🚀 Run Simulation'}
            </button>
            <button onClick={reset} className="btn btn-secondary">
              🔄 Reset
            </button>
          </div>
        </div>

        {/* Player Controls */}
        {results && mode === 'animate' && (
          <div className="player-controls">
            <div className="player-buttons">
              <button
                onClick={stepBack}
                disabled={currentStep <= 0}
                className="control-btn step-btn"
                title="Step Back"
              >
                ⏮
              </button>
              
              <button
                onClick={() => setIsPlaying(!isPlaying)}
                className="control-btn play-btn"
                title={isPlaying ? 'Pause' : 'Play'}
              >
                {isPlaying ? '⏸' : '▶️'}
              </button>
              
              <button
                onClick={stepForward}
                disabled={!results || currentStep >= (results[0]?.steps?.length || 0) - 1}
                className="control-btn step-btn"
                title="Step Forward"
              >
                ⏭
              </button>
            </div>

            <div className="player-info">
              <div className="speed-control">
                <label>Speed:</label>
                <input
                  type="range"
                  min="0.25"
                  max="2"
                  step="0.25"
                  value={speed}
                  onChange={(e) => setSpeed(parseFloat(e.target.value))}
                  className="speed-slider"
                />
                <span className="speed-value">{speed}x</span>
              </div>
              
              <div className="step-counter">
                Step {currentStep + 1} of {results[0]?.steps?.length || 0}
              </div>
            </div>
          </div>
        )}

        {/* Visualization */}
        <div className="visualization-panel">
          {!results ? (
            <div className="placeholder">
              <div className="placeholder-icon">📊</div>
              <p>Enter a trace and click "Run Simulation" to start</p>
            </div>
          ) : (
            <div className="results-container">
              {results.map((result, index) => (
                <div key={index} className="policy-result">
                  <div className="policy-header">
                    <h3 className="policy-title">{result.policy}</h3>
                    <div className="policy-stats">
                      <span className="stat-item">
                        <span className="stat-label">Hits:</span>
                        <span className="stat-value hit">{result.stats.hits}</span>
                      </span>
                      <span className="stat-item">
                        <span className="stat-label">Misses:</span>
                        <span className="stat-value miss">{result.stats.misses}</span>
                      </span>
                      <span className="stat-item">
                        <span className="stat-label">Ratio:</span>
                        <span className="stat-value">{(result.stats.hitRatio * 100).toFixed(1)}%</span>
                      </span>
                      <span className="stat-item">
                        <span className="stat-label">Evictions:</span>
                        <span className="stat-value">{result.stats.evictions}</span>
                      </span>
                    </div>
                  </div>

                  <div className="cache-visualization">
                    {Array.from({ length: result.capacity }, (_, i) => {
                      const currentStepData = getCurrentStepData();
                      const cache = currentStepData?.cache || [];
                      const item = cache[i];
                      
                      return (
                        <div
                          key={i}
                          className={`cache-box ${!item ? 'empty' : ''} ${
                            currentStepData?.key === item?.key
                              ? currentStepData.hit ? 'hit' : 'miss'
                              : ''
                          }`}
                        >
                          {item ? (
                            <>
                              <div className="cache-key">{item.key}</div>
                              <div className="cache-value">{item.value}</div>
                              {result.policy === 'LFU' && currentStepData?.meta?.freq?.[item.key] && (
                                <div className="freq-badge">
                                  {currentStepData.meta.freq[item.key]}
                                </div>
                              )}
                            </>
                          ) : (
                            <span className="empty-text">Empty</span>
                          )}
                        </div>
                      );
                    })}
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>

        {/* Comparison Table */}
        {results && (
          <div className="comparison-panel">
            <h3>Comparison Summary</h3>
            <div className="comparison-table">
              <table>
                <thead>
                  <tr>
                    <th>Policy</th>
                    <th>Hits</th>
                    <th>Misses</th>
                    <th>Hit Ratio</th>
                    <th>Evictions</th>
                  </tr>
                </thead>
                <tbody>
                  {results.map((result, index) => (
                    <tr key={index}>
                      <td>{result.policy}</td>
                      <td className="hit">{result.stats.hits}</td>
                      <td className="miss">{result.stats.misses}</td>
                      <td>{(result.stats.hitRatio * 100).toFixed(1)}%</td>
                      <td>{result.stats.evictions}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

export default App;

