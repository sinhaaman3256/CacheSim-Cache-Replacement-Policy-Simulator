#pragma once

#include "../include/types.hpp"
#include <queue>
#include <unordered_map>

namespace cachesim {

class FIFOPolicy : public IPolicy {
private:
    size_t capacity_;
    std::queue<std::string> arrival_order_;
    std::unordered_map<std::string, std::string> key_value_map_;

public:
    explicit FIFOPolicy(size_t capacity) : capacity_(capacity) {}
    
    bool get(const std::string& key, std::string& outVal) override {
        auto it = key_value_map_.find(key);
        if (it == key_value_map_.end()) {
            return false; // miss
        }
        
        // Hit - return value but don't change order
        outVal = it->second;
        return true; // hit
    }
    
    std::optional<std::string> put(const std::string& key, const std::string& val) override {
        std::optional<std::string> evicted;
        
        if (key_value_map_.find(key) == key_value_map_.end()) {
            // New key
            if (key_value_map_.size() >= capacity_) {
                // Evict oldest (front of queue)
                evicted = arrival_order_.front();
                key_value_map_.erase(evicted.value());
                arrival_order_.pop();
            }
            
            // Add new key to queue and map
            arrival_order_.push(key);
        }
        
        // Update or insert value (order unchanged for existing keys)
        key_value_map_[key] = val;
        
        return evicted;
    }
    
    std::vector<std::pair<std::string, std::string>> snapshot() const override {
        std::vector<std::pair<std::string, std::string>> result;
        result.reserve(key_value_map_.size());
        
        // Create a copy of the queue to iterate
        std::queue<std::string> temp_queue = arrival_order_;
        std::unordered_map<std::string, std::string> temp_map = key_value_map_;
        
        while (!temp_queue.empty()) {
            std::string key = temp_queue.front();
            temp_queue.pop();
            
            auto it = temp_map.find(key);
            if (it != temp_map.end()) {
                result.emplace_back(key, it->second);
                temp_map.erase(it); // Remove to avoid duplicates
            }
        }
        
        return result;
    }
    
    bool isCacheHit(const std::string& key) const override {
        return key_value_map_.find(key) != key_value_map_.end();
    }
};

} // namespace cachesim
