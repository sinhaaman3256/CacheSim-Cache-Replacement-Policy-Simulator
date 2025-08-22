#include "simulator.hpp"
#include <algorithm>

namespace cachesim {

SimResult Simulator::run(const std::vector<TraceOp>& ops, IPolicy& policy, const SimConfig& cfg) {
    SimResult result;
    result.stats = Stats{};
    
    // Performance guardrail for animate mode
    if (cfg.animate && ops.size() > 20000) {
        // Auto-switch to fast mode for very large traces
        SimConfig fast_cfg = cfg;
        fast_cfg.animate = false;
        return run(ops, policy, fast_cfg);
    }
    
    for (size_t i = 0; i < ops.size(); ++i) {
        const auto& op = ops[i];
        bool hit = false;
        std::optional<std::string> evicted;
        
        if (op.kind == TraceOp::Kind::GET) {
            std::string value;
            
            // For ARC policy, we need to check cache state BEFORE calling get()
            bool wasInCache = policy.isCacheHit(op.key);
            
            hit = policy.get(op.key, value);
            
            // For ARC policy, we need to distinguish between cache hits and ghost hits
            if (hit) {
                if (wasInCache) {
                    result.stats.hits++;
                } else {
                    result.stats.misses++; // Ghost hit counts as miss for statistics
                }
            } else {
                result.stats.misses++;
            }
        } else { // PUT
            evicted = policy.put(op.key, op.value);
            if (evicted) {
                result.stats.evictions++;
            }
            // PUT operations don't count toward hit/miss ratio
            hit = false; // PUT operations are never hits for statistics
        }
        
        // Create step record
        Step step = createStep(i, op, hit, evicted, policy);
        
        if (cfg.animate) {
            // Record every step in animate mode
            result.steps.push_back(std::move(step));
        } else {
            // Record sparse snapshots in fast mode
            if (i % cfg.snapshotEvery == 0 || i == ops.size() - 1) {
                result.snapshots.push_back(std::move(step));
            }
        }
    }
    
    return result;
}

Step Simulator::createStep(int index, const TraceOp& op, bool hit, 
                          const std::optional<std::string>& evicted, 
                          const IPolicy& policy) {
    Step step;
    step.index = index;
    step.op = (op.kind == TraceOp::Kind::GET) ? "GET" : "PUT";
    step.key = op.key;
    step.value = op.value;
    step.hit = hit;
    step.evicted = evicted;
    step.cache = policy.snapshot();
    
    // Get policy-specific metadata for UI
    policy.metaForUI(step);
    
    return step;
}

} // namespace cachesim
