// JavaScript-based Cache Simulator (Fallback when WASM isn't working)
class JSCacheSimulator {
    constructor() {
        this.policies = {
            LRU: this.createLRUPolicy,
            FIFO: this.createFIFOPolicy,
            LFU: this.createLFUPolicy,
            ARC: this.createARCPolicy
        };
    }

    // LRU Policy Implementation
    createLRUPolicy(capacity) {
        const cache = new Map();
        const accessOrder = [];
        
        return {
            get: (key) => {
                if (cache.has(key)) {
                    // Move to front (most recently used)
                    const index = accessOrder.indexOf(key);
                    accessOrder.splice(index, 1);
                    accessOrder.unshift(key);
                    return { hit: true, value: cache.get(key) };
                }
                return { hit: false, value: null };
            },
            put: (key, value) => {
                let evicted = null;
                
                if (cache.has(key)) {
                    // Update existing
                    cache.set(key, value);
                    const index = accessOrder.indexOf(key);
                    accessOrder.splice(index, 1);
                    accessOrder.unshift(key);
                } else {
                    // Check if we need to evict
                    if (cache.size >= capacity) {
                        const oldestKey = accessOrder.pop();
                        evicted = oldestKey;
                        cache.delete(oldestKey);
                    }
                    
                    // Add new item
                    cache.set(key, value);
                    accessOrder.unshift(key);
                }
                
                return { evicted };
            },
            snapshot: () => {
                return Array.from(cache.entries()).map(([key, value]) => ({ key, value }));
            }
        };
    }

    // FIFO Policy Implementation
    createFIFOPolicy(capacity) {
        const cache = new Map();
        const queue = [];
        
        return {
            get: (key) => {
                if (cache.has(key)) {
                    return { hit: true, value: cache.get(key) };
                }
                return { hit: false, value: null };
            },
            put: (key, value) => {
                let evicted = null;
                
                if (!cache.has(key)) {
                    // Check if we need to evict
                    if (cache.size >= capacity) {
                        const oldestKey = queue.shift();
                        evicted = oldestKey;
                        cache.delete(oldestKey);
                    }
                    
                    // Add new item
                    cache.set(key, value);
                    queue.push(key);
                } else {
                    // Update existing
                    cache.set(key, value);
                }
                
                return { evicted };
            },
            snapshot: () => {
                return Array.from(cache.entries()).map(([key, value]) => ({ key, value }));
            }
        };
    }

    // LFU Policy Implementation
    createLFUPolicy(capacity) {
        const cache = new Map();
        const frequencies = new Map();
        const freqLists = new Map();
        
        return {
            get: (key) => {
                if (cache.has(key)) {
                    // Update frequency
                    const oldFreq = frequencies.get(key);
                    const newFreq = oldFreq + 1;
                    frequencies.set(key, newFreq);
                    
                    // Remove from old frequency list
                    const oldList = freqLists.get(oldFreq);
                    const index = oldList.indexOf(key);
                    oldList.splice(index, 1);
                    if (oldList.length === 0) {
                        freqLists.delete(oldFreq);
                    }
                    
                    // Add to new frequency list
                    if (!freqLists.has(newFreq)) {
                        freqLists.set(newFreq, []);
                    }
                    freqLists.get(newFreq).push(key);
                    
                    return { hit: true, value: cache.get(key) };
                }
                return { hit: false, value: null };
            },
            put: (key, value) => {
                let evicted = null;
                
                if (!cache.has(key)) {
                    // Check if we need to evict
                    if (cache.size >= capacity) {
                        // Find lowest frequency
                        const minFreq = Math.min(...freqLists.keys());
                        const minList = freqLists.get(minFreq);
                        const oldestKey = minList.shift();
                        
                        if (minList.length === 0) {
                            freqLists.delete(minFreq);
                        }
                        
                        evicted = oldestKey;
                        cache.delete(oldestKey);
                        frequencies.delete(oldestKey);
                    }
                    
                    // Add new item
                    cache.set(key, value);
                    frequencies.set(key, 1);
                    if (!freqLists.has(1)) {
                        freqLists.set(1, []);
                    }
                    freqLists.get(1).push(key);
                } else {
                    // Update existing
                    cache.set(key, value);
                }
                
                return { evicted };
            },
            snapshot: () => {
                return Array.from(cache.entries()).map(([key, value]) => ({ key, value }));
            },
            getFrequencies: () => {
                return Object.fromEntries(frequencies);
            }
        };
    }

    // ARC Policy Implementation (Simplified)
    createARCPolicy(capacity) {
        const T1 = []; // Recently accessed items
        const T2 = []; // Frequently accessed items
        const B1 = []; // Ghost list for T1
        const B2 = []; // Ghost list for T2
        const cache = new Map();
        let p = 0; // Target size for T1
        
        return {
            get: (key) => {
                if (cache.has(key)) {
                    // Move to T2 if in T1
                    const t1Index = T1.indexOf(key);
                    if (t1Index !== -1) {
                        T1.splice(t1Index, 1);
                        T2.unshift(key);
                        if (T1.length < p) p = Math.max(0, p - 1);
                    }
                    return { hit: true, value: cache.get(key) };
                }
                
                // Check ghost lists
                const b1Index = B1.indexOf(key);
                const b2Index = B2.indexOf(key);
                
                if (b1Index !== -1) {
                    // Hit in B1, increase p
                    B1.splice(b1Index, 1);
                    p = Math.min(capacity, p + 1);
                    this.replace(key, null);
                } else if (b2Index !== -1) {
                    // Hit in B2, decrease p
                    B2.splice(b2Index, 1);
                    p = Math.max(0, p - 1);
                    this.replace(key, null);
                }
                
                return { hit: false, value: null };
            },
            
            replace: (key, value) => {
                let evicted = null;
                
                if (T1.length + T2.length >= capacity) {
                    if (T1.length > p) {
                        // Evict from T1
                        evicted = T1.pop();
                        cache.delete(evicted);
                        B1.unshift(evicted);
                    } else {
                        // Evict from T2
                        evicted = T2.pop();
                        cache.delete(evicted);
                        B2.unshift(evicted);
                    }
                }
                
                // Add to T1
                T1.unshift(key);
                cache.set(key, value);
                
                return { evicted };
            },
            
            put: (key, value) => {
                let evicted = null;
                
                if (cache.has(key)) {
                    // Update existing
                    cache.set(key, value);
                    this.get(key, value); // This will move it to T2
                } else {
                    evicted = this.replace(key, value);
                }
                
                return { evicted };
            },
            
            snapshot: () => {
                return Array.from(cache.entries()).map(([key, value]) => ({ key, value }));
            },
            
            getArcSets: () => {
                return { T1, T2, B1, B2, p };
            }
        };
    }

    // Main simulation function
    runSimulation(request) {
        const { capacity, policies, animate, traceText } = request;
        
        // Parse trace
        const operations = this.parseTrace(traceText);
        if (!operations.length) {
            return { error: "Failed to parse trace" };
        }
        
        const results = [];
        
        for (const policyName of policies) {
            if (!this.policies[policyName]) {
                continue;
            }
            
            const policy = this.policies[policyName](capacity);
            const steps = [];
            let hits = 0;
            let misses = 0;
            let evictions = 0;
            
            for (let i = 0; i < operations.length; i++) {
                const op = operations[i];
                let step = {
                    index: i,
                    op: op.type,
                    key: op.key,
                    value: op.value,
                    hit: false,
                    evicted: null,
                    cache: policy.snapshot(),
                    meta: {}
                };
                
                if (op.type === 'GET') {
                    const result = policy.get(op.key);
                    step.hit = result.hit;
                    if (result.hit) {
                        hits++;
                    } else {
                        misses++;
                    }
                } else if (op.type === 'PUT') {
                    const result = policy.put(op.key, op.value);
                    if (result.evicted) {
                        evictions++;
                        step.evicted = result.evicted;
                    }
                }
                
                // Add metadata
                if (policyName === 'LFU' && policy.getFrequencies) {
                    step.meta.freq = policy.getFrequencies();
                } else if (policyName === 'ARC' && policy.getArcSets) {
                    step.meta.arcSets = policy.getArcSets();
                }
                
                steps.push(step);
            }
            
            const hitRatio = hits + misses > 0 ? hits / (hits + misses) : 0;
            
            results.push({
                policy: policyName,
                capacity: capacity,
                steps: steps,
                stats: {
                    hits: hits,
                    misses: misses,
                    hitRatio: hitRatio,
                    evictions: evictions
                }
            });
        }
        
        return results.length === 1 ? results[0] : results;
    }
    
    // Parse trace text
    parseTrace(traceText) {
        const lines = traceText.trim().split('\n');
        const operations = [];
        
        for (const line of lines) {
            const trimmed = line.trim();
            if (!trimmed) continue;
            
            const parts = trimmed.split(' ');
            if (parts.length < 2) continue;
            
            const type = parts[0].toUpperCase();
            const key = parts[1];
            const value = parts.slice(2).join(' ');
            
            if (type === 'GET' || type === 'PUT') {
                operations.push({ type, key, value });
            }
        }
        
        return operations;
    }
}

// Make it globally available
window.JSCacheSimulator = JSCacheSimulator;
