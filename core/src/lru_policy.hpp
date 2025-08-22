#pragma once

#include "../include/types.hpp"
#include <list>
#include <unordered_map>

namespace cachesim {

class LRUPolicy : public IPolicy {
private:
    struct Node {
        std::string key;
        std::string value;
        Node(const std::string& k, const std::string& v) : key(k), value(v) {}
    };
    
    size_t capacity_;
    std::list<Node> recency_list_; // MRU -> ... -> LRU
    std::unordered_map<std::string, std::list<Node>::iterator> key_map_;

public:
    explicit LRUPolicy(size_t capacity) : capacity_(capacity) {}
    
    bool get(const std::string& key, std::string& outVal) override {
        auto it = key_map_.find(key);
        if (it == key_map_.end()) {
            return false; // miss
        }
        
        // Hit - move to front (MRU)
        auto node_it = it->second;
        outVal = node_it->value;
        
        // Move to front
        recency_list_.splice(recency_list_.begin(), recency_list_, node_it);
        
        return true; // hit
    }
    
    std::optional<std::string> put(const std::string& key, const std::string& val) override {
        auto it = key_map_.find(key);
        std::optional<std::string> evicted;
        
        if (it != key_map_.end()) {
            // Key exists - update value and move to front
            auto node_it = it->second;
            node_it->value = val;
            recency_list_.splice(recency_list_.begin(), recency_list_, node_it);
        } else {
            // New key
            if (recency_list_.size() >= capacity_) {
                // Evict LRU (back of list)
                auto lru_node = recency_list_.back();
                evicted = lru_node.key;
                key_map_.erase(lru_node.key);
                recency_list_.pop_back();
            }
            
            // Insert new node at front
            recency_list_.emplace_front(key, val);
            key_map_[key] = recency_list_.begin();
        }
        
        return evicted;
    }
    
    std::vector<std::pair<std::string, std::string>> snapshot() const override {
        std::vector<std::pair<std::string, std::string>> result;
        result.reserve(recency_list_.size());
        
        for (const auto& node : recency_list_) {
            result.emplace_back(node.key, node.value);
        }
        
        return result;
    }
    
    bool isCacheHit(const std::string& key) const override {
        return key_map_.find(key) != key_map_.end();
    }
};

} // namespace cachesim
