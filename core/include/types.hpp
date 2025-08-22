#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace cachesim {

struct ArcMeta {
    std::vector<std::string> T1, T2, B1, B2;
    int p;
};

struct Step {
    int index;
    std::string op;           // "GET" | "PUT"
    std::string key;
    std::string value;        // empty for GET
    bool hit;
    std::optional<std::string> evicted; // key evicted
    // Cache state after this step
    std::vector<std::pair<std::string, std::string>> cache; // in display order
    // optional metadata for UI (freqs, ARC sets)
    std::unordered_map<std::string, int> freq; 
    std::optional<ArcMeta> arc;
};

struct Stats {
    uint64_t hits = 0, misses = 0, evictions = 0;
    
    double hitRatio() const { 
        auto total = hits + misses; 
        return total ? double(hits) / double(total) : 0.0; 
    }
};

struct TraceOp {
    enum class Kind { GET, PUT };
    Kind kind;
    std::string key;
    std::string value; // empty if GET
};

class IPolicy {
public:
    virtual ~IPolicy() = default;
    virtual bool get(const std::string& key, std::string& outVal) = 0;
    virtual std::optional<std::string> put(const std::string& key, const std::string& val) = 0;
    virtual std::vector<std::pair<std::string, std::string>> snapshot() const = 0; // display order
    virtual void metaForUI(Step& s) const { (void)s; } // optional (LFU freq, ARC sets)
    
    // For ARC policy to distinguish between cache hits and ghost hits
    virtual bool isCacheHit(const std::string& key) const { (void)key; return false; }
};

struct SimConfig {
    size_t capacity;
    bool animate;           // true = record every step
    size_t snapshotEvery;   // e.g., 1000 for fast mode
};

struct SimResult {
    std::vector<Step> steps;     // empty if fast mode
    std::vector<Step> snapshots; // sparse steps if fast mode
    Stats stats;
};

} // namespace cachesim
