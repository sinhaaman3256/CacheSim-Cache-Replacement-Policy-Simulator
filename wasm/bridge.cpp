#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <string>
#include <memory>
#include <vector>

#include "../core/include/types.hpp"
#include "../core/src/lru_policy.hpp"
#include "../core/src/fifo_policy.hpp"
#include "../core/src/lfu_policy.hpp"
#include "../core/src/arc_policy.hpp"
#include "../core/src/simulator.hpp"
#include "../core/src/trace_parser.hpp"
#include <emscripten/emscripten.h>

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    const char* run_simulation_json(const char* requestJson);

    EMSCRIPTEN_KEEPALIVE
    void free_json(const char* ptr);
}


using namespace cachesim;

extern "C" {

// C-style interface for WASM
const char* run_simulation_json(const char* requestJson);
void free_json(const char* ptr);

}

// Helper function to create policy instance
std::unique_ptr<IPolicy> createPolicy(const std::string& policyName, size_t capacity) {
    if (policyName == "LRU") {
        return std::make_unique<LRUPolicy>(capacity);
    } else if (policyName == "FIFO") {
        return std::make_unique<FIFOPolicy>(capacity);
    } else if (policyName == "LFU") {
        return std::make_unique<LFUPolicy>(capacity);
    } else if (policyName == "ARC") {
        return std::make_unique<ARCPolicy>(capacity);
    } else {
        throw std::runtime_error("Unknown policy: " + policyName);
    }
}

// Simple JSON serialization (for simplicity, using basic string building)
std::string serializeStep(const Step& step) {
    std::string result = "{";
    result += "\"index\":" + std::to_string(step.index) + ",";
    result += "\"op\":\"" + step.op + "\",";
    result += "\"key\":\"" + step.key + "\",";
    result += "\"value\":\"" + step.value + "\",";
    result += "\"hit\":" + std::string(step.hit ? "true" : "false") + ",";

    
    if (step.evicted) {
        result += "\"evicted\":\"" + step.evicted.value() + "\",";
    } else {
        result += "\"evicted\":null,";
    }
    
    result += "\"cache\":[";
    for (size_t i = 0; i < step.cache.size(); ++i) {
        if (i > 0) result += ",";
        result += "{\"key\":\"" + step.cache[i].first + "\",\"value\":\"" + step.cache[i].second + "\"}";
    }
    result += "],";
    
    result += "\"meta\":{";
    result += "\"freq\":{";
    bool first = true;
    for (const auto& [key, freq] : step.freq) {
        if (!first) result += ",";
        result += "\"" + key + "\":" + std::to_string(freq);
        first = false;
    }
    result += "},";
    
    if (step.arc) {
        result += "\"arcSets\":{";
        result += "\"T1\":[";
        for (size_t i = 0; i < step.arc->T1.size(); ++i) {
            if (i > 0) result += ",";
            result += "\"" + step.arc->T1[i] + "\"";
        }
        result += "],\"T2\":[";
        for (size_t i = 0; i < step.arc->T2.size(); ++i) {
            if (i > 0) result += ",";
            result += "\"" + step.arc->T2[i] + "\"";
        }
        result += "],\"B1\":[";
        for (size_t i = 0; i < step.arc->B1.size(); ++i) {
            if (i > 0) result += ",";
            result += "\"" + step.arc->B1[i] + "\"";
        }
        result += "],\"B2\":[";
        for (size_t i = 0; i < step.arc->B2.size(); ++i) {
            if (i > 0) result += ",";
            result += "\"" + step.arc->B2[i] + "\"";
        }
        result += "],\"p\":" + std::to_string(step.arc->p);
        result += "}";
    } else {
        result += "\"arcSets\":null";
    }
    result += "}";
    result += "}";
    
    return result;
}

std::string serializeStats(const Stats& stats) {
    std::string result = "{";
    result += "\"hits\":" + std::to_string(stats.hits) + ",";
    result += "\"misses\":" + std::to_string(stats.misses) + ",";
    result += "\"hitRatio\":" + std::to_string(stats.hitRatio()) + ",";
    result += "\"evictions\":" + std::to_string(stats.evictions);
    result += "}";
    return result;
}

std::string serializeResult(const SimResult& result, const std::string& policyName, size_t capacity) {
    std::string json = "{";
    json += "\"policy\":\"" + policyName + "\",";
    json += "\"capacity\":" + std::to_string(capacity) + ",";
    
    if (!result.steps.empty()) {
        json += "\"steps\":[";
        for (size_t i = 0; i < result.steps.size(); ++i) {
            if (i > 0) json += ",";
            json += serializeStep(result.steps[i]);
        }
        json += "],";
    }
    
    if (!result.snapshots.empty()) {
        json += "\"snapshots\":[";
        for (size_t i = 0; i < result.snapshots.size(); ++i) {
            if (i > 0) json += ",";
            json += serializeStep(result.snapshots[i]);
        }
        json += "],";
    }
    
    json += "\"stats\":" + serializeStats(result.stats);
    json += "}";
    
    return json;
}

// Simple JSON parsing (basic implementation)
struct JsonRequest {
    size_t capacity;
    std::vector<std::string> policies;
    bool animate;
    size_t snapshotEvery;
    std::string traceText;
};

JsonRequest parseJsonRequest(const std::string& jsonStr) {
    JsonRequest req;
    req.capacity = 3;
    req.animate = true;
    req.snapshotEvery = 1000;
    
    // Debug: print the raw JSON we received
    printf("DEBUG: Raw JSON received: %s\n", jsonStr.c_str());
    
    // Extract capacity
    size_t capPos = jsonStr.find("\"capacity\":");
    if (capPos != std::string::npos) {
        size_t start = capPos + 11;
        size_t end = jsonStr.find_first_not_of("0123456789", start);
        if (end != std::string::npos) {
            req.capacity = std::stoul(jsonStr.substr(start, end - start));
            printf("DEBUG: Extracted capacity: %zu\n", req.capacity);
        }
    }
    
    // Extract animate
    size_t animPos = jsonStr.find("\"animate\":");
    if (animPos != std::string::npos) {
        req.animate = jsonStr.find("true", animPos) != std::string::npos;
        printf("DEBUG: Extracted animate: %s\n", req.animate ? "true" : "false");
    }
    
    // Extract snapshotEvery
    size_t snapPos = jsonStr.find("\"snapshotEvery\":");
    if (snapPos != std::string::npos) {
        size_t start = snapPos + 16;
        size_t end = jsonStr.find_first_not_of("0123456789", start);
        if (end != std::string::npos) {
            req.snapshotEvery = std::stoul(jsonStr.substr(start, end - start));
            printf("DEBUG: Extracted snapshotEvery: %zu\n", req.snapshotEvery);
        }
    }
    
    // Extract policies
    size_t policiesPos = jsonStr.find("\"policies\":");
    if (policiesPos != std::string::npos) {
        size_t start = jsonStr.find("[", policiesPos);
        size_t end = jsonStr.find("]", start);
        if (start != std::string::npos && end != std::string::npos) {
            std::string policiesStr = jsonStr.substr(start + 1, end - start - 1);
            size_t pos = 0;
            while ((pos = policiesStr.find("\"", pos)) != std::string::npos) {
                size_t endQuote = policiesStr.find("\"", pos + 1);
                if (endQuote != std::string::npos) {
                    req.policies.push_back(policiesStr.substr(pos + 1, endQuote - pos - 1));
                    pos = endQuote + 1;
                } else {
                    break;
                }
            }
            printf("DEBUG: Extracted %zu policies\n", req.policies.size());
        }
    }
    
    // Extract traceText - completely rewritten approach
    size_t tracePos = jsonStr.find("\"traceText\":");
    if (tracePos != std::string::npos) {
        printf("DEBUG: Found traceText at position %zu\n", tracePos);
        
        // Find the opening quote after "traceText":
        size_t start = jsonStr.find("\"", tracePos + 12);
        if (start != std::string::npos) {
            printf("DEBUG: Opening quote found at position %zu\n", start);
            
            // Now we need to find the closing quote, but we need to handle escaped quotes
            // For now, let's try a simple approach: find the next unescaped quote
            size_t end = start + 1;
            bool foundEnd = false;
            
            while (end < jsonStr.length() && !foundEnd) {
                end = jsonStr.find("\"", end);
                if (end != std::string::npos) {
                    // Check if this quote is escaped (preceded by backslash)
                    if (end == 0 || jsonStr[end - 1] != '\\') {
                        // This is an unescaped quote, so it's our end
                        foundEnd = true;
                        printf("DEBUG: Closing quote found at position %zu\n", end);
                    } else {
                        // This is an escaped quote, continue searching
                        end++;
                    }
                } else {
                    break;
                }
            }
            
            if (foundEnd) {
                req.traceText = jsonStr.substr(start + 1, end - start - 1);
                printf("DEBUG: Extracted traceText: '%s'\n", req.traceText.c_str());
                printf("DEBUG: traceText length: %zu\n", req.traceText.length());
            } else {
                printf("DEBUG: ERROR: Could not find closing quote for traceText\n");
            }
        } else {
            printf("DEBUG: ERROR: Could not find opening quote after traceText:\n");
        }
    } else {
        printf("DEBUG: ERROR: Could not find traceText field\n");
    }
    
    return req;
}

const char* run_simulation_json(const char* requestJson) {
    try {
        std::string jsonStr(requestJson);
        JsonRequest req = parseJsonRequest(jsonStr);
        
        // Debug: print what we extracted
        std::string debugInfo = "Extracted: capacity=" + std::to_string(req.capacity) + 
                               ", policies=" + std::to_string(req.policies.size()) + 
                               ", traceText='" + req.traceText + "'";
        
        // Parse trace
        ParseResult parseResult = TraceParser::parse(req.traceText);
        
        // Debug: check what was parsed
        std::string parseDebug = "Parsed " + std::to_string(parseResult.operations.size()) + " operations";
        for (size_t i = 0; i < parseResult.operations.size(); ++i) {
            parseDebug += "\n  " + std::to_string(i) + ": " + 
                         (parseResult.operations[i].kind == TraceOp::Kind::GET ? "GET" : "PUT") + 
                         " " + parseResult.operations[i].key;
            if (parseResult.operations[i].kind == TraceOp::Kind::PUT) {
                parseDebug += " " + parseResult.operations[i].value;
            }
        }
        
        if (!parseResult.success) {
            std::string errorJson = "{\"error\":\"Parse failed\",\"details\":[";
            for (size_t i = 0; i < parseResult.errors.size(); ++i) {
                if (i > 0) errorJson += ",";
                errorJson += "\"" + parseResult.errors[i] + "\"";
            }
            errorJson += "],\"debug\":\"" + debugInfo + "\",\"parseDebug\":\"" + parseDebug + "\"}";
            
            char* result = static_cast<char*>(malloc(errorJson.length() + 1));
            strcpy(result, errorJson.c_str());
            return result;
        }
        
        // Debug: check if operations were parsed
        if (parseResult.operations.empty()) {
            std::string errorJson = "{\"error\":\"No operations parsed from trace\",\"traceText\":\"" + req.traceText + "\",\"debug\":\"" + debugInfo + "\",\"parseDebug\":\"" + parseDebug + "\"}";
            char* result = static_cast<char*>(malloc(errorJson.length() + 1));
            strcpy(result, errorJson.c_str());
            return result;
        }
        
        // Validate capacity
        if (req.capacity == 0) {
            std::string errorJson = "{\"error\":\"Capacity must be greater than 0\"}";
            char* result = static_cast<char*>(malloc(errorJson.length() + 1));
            strcpy(result, errorJson.c_str());
            return result;
        }
        
        // If no policies specified, default to LRU
        if (req.policies.empty()) {
            req.policies.push_back("LRU");
        }
        
        // Run simulation for each policy
        if (req.policies.size() == 1) {
            // Single policy
            auto policy = createPolicy(req.policies[0], req.capacity);
            Simulator simulator;
            SimConfig config{req.capacity, req.animate, req.snapshotEvery};
            SimResult result = simulator.run(parseResult.operations, *policy, config);
            
            std::string json = serializeResult(result, req.policies[0], req.capacity);
            char* jsonResult = static_cast<char*>(malloc(json.length() + 1));
            strcpy(jsonResult, json.c_str());
            return jsonResult;
        } else {
            // Multiple policies - comparison mode
            std::string json = "[";
            for (size_t i = 0; i < req.policies.size(); ++i) {
                if (i > 0) json += ",";
                
                auto policy = createPolicy(req.policies[i], req.capacity);
                Simulator simulator;
                SimConfig config{req.capacity, req.animate, req.snapshotEvery};
                SimResult result = simulator.run(parseResult.operations, *policy, config);
                
                json += serializeResult(result, req.policies[i], req.capacity);
            }
            json += "]";
            
            char* jsonResult = static_cast<char*>(malloc(json.length() + 1));
            strcpy(jsonResult, json.c_str());
            return jsonResult;
        }
        
    } catch (const std::exception& e) {
        std::string errorJson = "{\"error\":\"Simulation failed: " + std::string(e.what()) + "\"}";
        char* result = static_cast<char*>(malloc(errorJson.length() + 1));
        strcpy(result, errorJson.c_str());
        return result;
    }
}

void free_json(const char* ptr) {
    if (ptr) {
        free(const_cast<char*>(ptr));
    }
}

// // Emscripten bindings for easier debugging
// EMSCRIPTEN_BINDINGS(cachesim_module) {
//     emscripten::function("runSimulation", &run_simulation_json);
//     emscripten::function("freeResult", &free_json);
// }
