// CacheSim - Cache Replacement Policy Simulator
// Uses C++ backend compiled to WebAssembly (WASM) for high-performance cache simulation
// JavaScript handles UI, WASM handles all the computational logic
console.log("🚀 CacheSim initializing with WASM backend...");

class CacheSimulator {
    constructor() {
        console.log('🚀 Initializing CacheSimulator...');
        this.wasmModule = null;
        this.currentResults = null;
        this.currentStep = 0;
        this.isPlaying = false;
        this.playInterval = null;
        this.speed = 1.0;
        
        this.initializeElements();
        this.bindEvents();
        this.initializeWASM();
        console.log('✅ CacheSimulator initialized');
    }
    
    initializeElements() {
        this.elements = {
            capacity: document.getElementById('capacity'),
            policies: document.querySelectorAll('input[type="checkbox"][value]'),
            modeRadios: document.querySelectorAll('input[name="mode"]'),
            snapshotControl: document.getElementById('snapshot-control'),
            snapshotEvery: document.getElementById('snapshotEvery'),
            traceInput: document.getElementById('traceInput'),
            runBtn: document.getElementById('runBtn'),
            resetBtn: document.getElementById('resetBtn'),
            playerBar: document.getElementById('playerBar'),
            stepBackBtn: document.getElementById('stepBackBtn'),
            playPauseBtn: document.getElementById('playPauseBtn'),
            stepForwardBtn: document.getElementById('stepForwardBtn'),
            speedSlider: document.getElementById('speedSlider'),
            speedValue: document.getElementById('speedValue'),
            currentStepSpan: document.getElementById('currentStep'),
            totalStepsSpan: document.getElementById('totalSteps'),
            visualizationArea: document.getElementById('visualizationArea'),
            comparisonSummary: document.getElementById('comparisonSummary'),
            comparisonTable: document.getElementById('comparisonTable')
        };
    }
    
    bindEvents() {
        this.elements.runBtn.addEventListener('click', () => this.runSimulation());
        this.elements.resetBtn.addEventListener('click', () => this.reset());
        
        this.elements.modeRadios.forEach(radio => {
            radio.addEventListener('change', (e) => {
                if (e.target.value === 'fast') {
                    this.elements.snapshotControl.style.display = 'block';
                } else {
                    this.elements.snapshotControl.style.display = 'none';
                }
            });
        });
        
        this.elements.stepBackBtn.addEventListener('click', () => this.stepBack());
        this.elements.playPauseBtn.addEventListener('click', () => this.togglePlay());
        this.elements.stepForwardBtn.addEventListener('click', () => this.stepForward());
        
        this.elements.speedSlider.addEventListener('input', (e) => {
            this.speed = parseFloat(e.target.value);
            this.elements.speedValue.textContent = this.speed + '×';
            if (this.isPlaying) {
                this.startPlayback();
            }
        });
    }
    
    async initializeWASM() {
        try {
            this.elements.runBtn.textContent = 'Loading WASM...';
            this.elements.runBtn.disabled = true;
            
            // Wait for WASM module to be ready
            let attempts = 0;
            const maxAttempts = 50; // 5 seconds max
            
            while (attempts < maxAttempts) {
                if (window.Module && window.Module._run_simulation_json && 
                    window.Module.stringToUTF8 && window.Module.UTF8ToString) {
                    
                    this.wasmModule = window.Module;
                    console.log('✅ WASM module loaded successfully with functions:', 
                        Object.keys(this.wasmModule).filter(k => k.startsWith('_')));
                    console.log('Runtime methods available:', 
                        Object.keys(this.wasmModule).filter(k => ['stringToUTF8', 'UTF8ToString'].includes(k)));
                    this.elements.runBtn.textContent = 'Run Simulation';
                    this.elements.runBtn.disabled = false;
                    return;
                }
                
                await new Promise(resolve => setTimeout(resolve, 100));
                attempts++;
            }
            
            throw new Error('WASM module failed to load within timeout');
            
        } catch (error) {
            console.error('Failed to initialize WASM:', error);
            this.elements.runBtn.textContent = 'WASM Failed';
            this.elements.runBtn.disabled = true;
            alert('Failed to load WebAssembly module. Please refresh the page.');
        }
    }
    
    
    async runSimulation() {
        try {
            this.elements.runBtn.textContent = 'Running...';
            this.elements.runBtn.disabled = true;
    
            // Get configuration
            const capacity = parseInt(this.elements.capacity.value);
            const policies = Array.from(this.elements.policies)
                .filter(cb => cb.checked)
                .map(cb => cb.value);
            const animate = this.elements.modeRadios[0].checked;
            const snapshotEvery = parseInt(this.elements.snapshotEvery.value);
            const traceText = this.elements.traceInput.value.trim();
    
            if (capacity <= 0) {
                alert('Capacity must be greater than 0');
                return;
            }
    
            if (policies.length === 0) {
                alert('Please select at least one policy');
                return;
            }
    
            if (traceText.trim() === '') {
                alert('Please enter a trace');
                return;
            }
    
            // Prepare request
            const request = {
                capacity,
                policies,
                animate,
                snapshotEvery,
                traceText
            };
            
            console.log('Request:', request);
    
            let result;
            
            // Run WASM simulation
            console.log('🚀 Running WASM simulation...');
            console.log('WASM module functions:', Object.keys(this.wasmModule).filter(k => k.startsWith('_')));
            
            // Prepare request for WASM
            const requestJson = JSON.stringify(request);
            console.log('WASM Request JSON:', requestJson);
            
            // Check if functions exist
            if (!this.wasmModule._malloc) {
                throw new Error('_malloc function not found');
            }
            if (!this.wasmModule._run_simulation_json) {
                throw new Error('_run_simulation_json function not found');
            }
            if (!this.wasmModule.stringToUTF8) {
                throw new Error('stringToUTF8 function not found');
            }
            if (!this.wasmModule.UTF8ToString) {
                throw new Error('UTF8ToString function not found');
            }
            
            // Allocate memory for the request
            const requestPtr = this.wasmModule._malloc(requestJson.length + 1);
            console.log('Allocated memory at:', requestPtr);
            
            // Copy string to WASM memory
            this.wasmModule.stringToUTF8(requestJson, requestPtr, requestJson.length + 1);
            console.log('String copied to memory');
            
            // Call WASM function
            console.log('Calling _run_simulation_json...');
            const resultPtr = this.wasmModule._run_simulation_json(requestPtr);
            console.log('Result pointer:', resultPtr);
            
            // Get result string
            const resultStr = this.wasmModule.UTF8ToString(resultPtr);
            console.log('WASM Raw Result:', resultStr);
            
            // Free memory
            this.wasmModule._free(requestPtr);
            this.wasmModule._free_json(resultPtr);
            
            // Parse result
            result = JSON.parse(resultStr);
            console.log('✅ WASM simulation successful');
            
            this.currentResults = Array.isArray(result) ? result : [result];
            this.currentStep = 0;

            this.renderVisualization();
            this.updatePlayerBar();

            if (animate) {
                this.elements.playerBar.style.display = 'flex';
                this.elements.comparisonSummary.style.display = 'block';
            } else {
                this.elements.playerBar.style.display = 'none';
                this.elements.comparisonSummary.style.display = 'block';
            }
            
        } catch (error) {
            console.error('Simulation failed:', error);
            alert('Simulation failed: ' + error.message);
            return;
        } finally {
            this.elements.runBtn.textContent = 'Run Simulation';
            this.elements.runBtn.disabled = false;
        }
    }
    
    
    renderVisualization() {
        if (!this.currentResults) return;
        
        this.elements.visualizationArea.innerHTML = '';
        
        this.currentResults.forEach((result, index) => {
            const policyRow = document.createElement('div');
            policyRow.className = 'policy-row';
            
            const title = document.createElement('div');
            title.className = 'policy-title';
            title.innerHTML = `
                <span>${result.policy}</span>
                <span class="policy-stats">
                    hits: ${result.stats.hits}, misses: ${result.stats.misses}, 
                    ratio: ${(result.stats.hitRatio * 100).toFixed(1)}%, evictions: ${result.stats.evictions}
                </span>
            `;
            
            const cacheRow = document.createElement('div');
            cacheRow.className = 'cache-row';
            
            // Create cache boxes
            const steps = result.steps || result.snapshots || [];
            if (steps.length > 0) {
                const currentStep = steps[Math.min(this.currentStep, steps.length - 1)];
                const cache = currentStep.cache;
                
                // Fill cache boxes
                for (let i = 0; i < result.capacity; i++) {
                    const box = document.createElement('div');
                    box.className = 'cache-box';
                    
                    if (i < cache.length) {
                        const item = cache[i];
                        box.innerHTML = `
                            <div class="cache-key">${item.key}</div>
                            <div class="cache-value">${item.value}</div>
                        `;
                        
                        // Add frequency badge for LFU
                        if (result.policy === 'LFU' && currentStep.meta && currentStep.meta.freq) {
                            const freq = currentStep.meta.freq[item.key];
                            if (freq) {
                                const badge = document.createElement('div');
                                badge.className = 'freq-badge';
                                badge.textContent = freq;
                                box.appendChild(badge);
                            }
                        }
                        
                        // Highlight current operation
                        if (currentStep.key === item.key) {
                            if (currentStep.hit) {
                                box.classList.add('hit');
                            } else {
                                box.classList.add('miss');
                            }
                        }
                    } else {
                        box.classList.add('empty');
                    }
                    
                    cacheRow.appendChild(box);
                }
                
                // Add ARC metadata if available
                if (result.policy === 'ARC' && currentStep.meta && currentStep.meta.arcSets) {
                    const arcMeta = document.createElement('div');
                    arcMeta.className = 'arc-meta';
                    
                    const sets = ['T1', 'T2', 'B1', 'B2'];
                    sets.forEach(setName => {
                        const setDiv = document.createElement('div');
                        setDiv.className = 'arc-set';
                        
                        const label = document.createElement('span');
                        label.className = 'arc-set-label';
                        label.textContent = setName + ':';
                        
                        const items = document.createElement('div');
                        items.className = 'arc-set-items';
                        
                        const setItems = currentStep.meta.arcSets[setName] || [];
                        setItems.forEach(item => {
                            const itemSpan = document.createElement('span');
                            itemSpan.className = 'arc-item';
                            itemSpan.textContent = item;
                            items.appendChild(itemSpan);
                        });
                        
                        setDiv.appendChild(label);
                        setDiv.appendChild(items);
                        arcMeta.appendChild(setDiv);
                    });
                    
                    // Add p value
                    const pDiv = document.createElement('div');
                    pDiv.className = 'arc-set';
                    pDiv.innerHTML = `<span class="arc-set-label">p:</span> <span class="arc-item">${currentStep.meta.arcSets.p}</span>`;
                    arcMeta.appendChild(pDiv);
                    
                    policyRow.appendChild(arcMeta);
                }
            }
            
            policyRow.appendChild(title);
            policyRow.appendChild(cacheRow);
            this.elements.visualizationArea.appendChild(policyRow);
        });
        
        this.updateComparisonTable();
    }
    
    updateComparisonTable() {
        if (!this.currentResults) return;
        
        const tbody = this.elements.comparisonTable.querySelector('tbody');
        tbody.innerHTML = '';
        
        this.currentResults.forEach(result => {
            const row = document.createElement('tr');
            row.innerHTML = `
                <td>${result.policy}</td>
                <td>${result.stats.hits}</td>
                <td>${result.stats.misses}</td>
                <td>${(result.stats.hitRatio * 100).toFixed(1)}%</td>
                <td>${result.stats.evictions}</td>
            `;
            tbody.appendChild(row);
        });
    }
    
    updatePlayerBar() {
        if (!this.currentResults || this.currentResults.length === 0) return;
        
        const result = this.currentResults[0];
        const steps = result.steps || result.snapshots || [];
        const totalSteps = steps.length;
        
        this.elements.totalStepsSpan.textContent = totalSteps;
        this.elements.currentStepSpan.textContent = this.currentStep + 1;
        
        // Update button states
        this.elements.stepBackBtn.disabled = this.currentStep <= 0;
        this.elements.stepForwardBtn.disabled = this.currentStep >= totalSteps - 1;
    }
    
    stepBack() {
        if (this.currentStep > 0) {
            this.currentStep--;
            this.renderVisualization();
            this.updatePlayerBar();
        }
    }
    
    stepForward() {
        if (this.currentResults && this.currentResults.length > 0) {
            const result = this.currentResults[0];
            const steps = result.steps || result.snapshots || [];
            if (this.currentStep < steps.length - 1) {
                this.currentStep++;
                this.renderVisualization();
                this.updatePlayerBar();
            }
        }
    }
    
    togglePlay() {
        if (this.isPlaying) {
            this.stopPlayback();
        } else {
            this.startPlayback();
        }
    }
    
    startPlayback() {
        this.isPlaying = true;
        this.elements.playPauseBtn.textContent = '⏸';
        
        const interval = Math.max(100, 1000 / this.speed);
        this.playInterval = setInterval(() => {
            if (this.currentStep < this.getMaxSteps() - 1) {
                this.currentStep++;
                this.renderVisualization();
                this.updatePlayerBar();
            } else {
                this.stopPlayback();
            }
        }, interval);
    }
    
    stopPlayback() {
        this.isPlaying = false;
        this.elements.playPauseBtn.textContent = '⏯';
        
        if (this.playInterval) {
            clearInterval(this.playInterval);
            this.playInterval = null;
        }
    }
    
    getMaxSteps() {
        if (!this.currentResults || this.currentResults.length === 0) return 0;
        const result = this.currentResults[0];
        const steps = result.steps || result.snapshots || [];
        return steps.length;
    }
    
    reset() {
        this.currentResults = null;
        this.currentStep = 0;
        this.stopPlayback();
        
        this.elements.visualizationArea.innerHTML = `
            <div class="placeholder">
                <p>Enter a trace and click "Run Simulation" to start</p>
            </div>
        `;
        
        this.elements.playerBar.style.display = 'none';
        this.elements.comparisonSummary.style.display = 'none';
    }
}

// Initialize the simulator when the page loads
document.addEventListener('DOMContentLoaded', () => {
    console.log('📄 DOM loaded, initializing CacheSimulator...');
    try {
        const simulator = new CacheSimulator();
        console.log('✅ CacheSimulator created successfully');
    } catch (error) {
        console.error('❌ Failed to create CacheSimulator:', error);
        alert('Failed to initialize simulator: ' + error.message);
    }
});
