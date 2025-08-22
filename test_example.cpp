#include <iostream>
#include <string>
#include <vector>

// Include our cache implementations
#include "core/include/types.hpp"
#include "core/src/lru_policy.hpp"
#include "core/src/fifo_policy.hpp"
#include "core/src/lfu_policy.hpp"
#include "core/src/arc_policy.hpp"
#include "core/src/simulator.hpp"
#include "core/src/trace_parser.hpp"

using namespace cachesim;

void testPolicy(const std::string& policyName, IPolicy& policy, const std::vector<TraceOp>& ops) {
    std::cout << "\n=== Testing " << policyName << " ===" << std::endl;
    
    Simulator simulator;
    SimConfig config{2, true, 1000}; // capacity=2, animate=true
    
    SimResult result = simulator.run(ops, policy, config);
    
    std::cout << "Final stats:" << std::endl;
    std::cout << "  Hits: " << result.stats.hits << std::endl;
    std::cout << "  Misses: " << result.stats.misses << std::endl;
    std::cout << "  Hit ratio: " << (result.stats.hitRatio() * 100) << "%" << std::endl;
    std::cout << "  Evictions: " << result.stats.evictions << std::endl;
    
    std::cout << "Step-by-step:" << std::endl;
    for (const auto& step : result.steps) {
        std::cout << "  Step " << step.index << ": " << step.op << " " << step.key;
        if (step.op == "PUT") {
            std::cout << " " << step.value;
        }
        std::cout << " -> " << (step.hit ? "HIT" : "MISS");
        if (step.evicted) {
            std::cout << " (evicted: " << step.evicted.value() << ")";
        }
        std::cout << std::endl;
        
        std::cout << "    Cache: [";
        for (size_t i = 0; i < step.cache.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << step.cache[i].first << ":" << step.cache[i].second;
        }
        std::cout << "]" << std::endl;
    }
}

int main() {
    std::cout << "CacheSim Test - Example from Spec" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Example trace from spec: Capacity=2, Policy=LRU
    std::string traceText = R"(
PUT A a
PUT B b
GET A
PUT C c
GET B
GET C
)";
    
    // Parse trace
    ParseResult parseResult = TraceParser::parse(traceText);
    if (!parseResult.success) {
        std::cout << "Parse failed:" << std::endl;
        for (const auto& error : parseResult.errors) {
            std::cout << "  " << error << std::endl;
        }
        return 1;
    }
    
    std::cout << "Parsed " << parseResult.operations.size() << " operations:" << std::endl;
    for (const auto& op : parseResult.operations) {
        std::cout << "  " << (op.kind == TraceOp::Kind::GET ? "GET" : "PUT") 
                  << " " << op.key;
        if (op.kind == TraceOp::Kind::PUT) {
            std::cout << " " << op.value;
        }
        std::cout << std::endl;
    }
    
    // Test each policy
    LRUPolicy lruPolicy(2);
    FIFOPolicy fifoPolicy(2);
    LFUPolicy lfuPolicy(2);
    ARCPolicy arcPolicy(2);
    
    testPolicy("LRU", lruPolicy, parseResult.operations);
    testPolicy("FIFO", fifoPolicy, parseResult.operations);
    testPolicy("LFU", lfuPolicy, parseResult.operations);
    testPolicy("ARC", arcPolicy, parseResult.operations);
    
    std::cout << "\nExpected results for this trace:" << std::endl;
    std::cout << "  LRU: hits=2 (GET A, GET C), misses=1 (GET B), evictions=1 (B)" << std::endl;
    std::cout << "  FIFO: hits=3 (GET A, GET B, GET C), misses=0, evictions=1 (A)" << std::endl;
    std::cout << "  LFU: hits=2 (GET A, GET C), misses=1 (GET B), evictions=1 (B)" << std::endl;
    std::cout << "  ARC: hits=2 (GET A, GET C), misses=1 (GET B), evictions=1 (B)" << std::endl;
    
    return 0;
}
