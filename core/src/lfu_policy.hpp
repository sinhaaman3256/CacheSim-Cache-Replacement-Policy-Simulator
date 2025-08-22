#pragma once

#include "../include/types.hpp"
#include <list>
#include <unordered_map>
#include <algorithm>

namespace cachesim {

class LFUPolicy : public IPolicy {
private:
    struct Node {
        std::string key;
        std::string value;
        int frequency;
        Node(const std::string& k, const std::string& v, int freq) 
            : key(k), value(v), frequency(freq) {}
    };
    
    size_t capacity_;
    int min_frequency_;
    std::unordered_map<std::string, std::list<Node>::iterator> key_map_;
    std::unordered_map<int, std::list<Node>> frequency_lists_;

public:
    explicit LFUPolicy(size_t capacity) : capacity_(capacity), min_frequency_(1) {}
    
    bool get(const std::string& key, std::string& outVal) override {
        auto it = key_map_.find(key);
        if (it == key_map_.end()) {
            return false; // miss
        }
        
        // Hit - increment frequency and move to appropriate list
        auto node_it = it->second;
        outVal = node_it->value;
        int old_freq = node_it->frequency;
        
        // Remove from old frequency list
        frequency_lists_[old_freq].erase(node_it);
        if (frequency_lists_[old_freq].empty()) {
            frequency_lists_.erase(old_freq);
            if (old_freq == min_frequency_) {
                min_frequency_ = old_freq + 1;
            }
        }
        
        // Insert into new frequency list
        int new_freq = old_freq + 1;
        frequency_lists_[new_freq].emplace_front(key, outVal, new_freq);
        key_map_[key] = frequency_lists_[new_freq].begin();
        
        return true; // hit
    }
    
    std::optional<std::string> put(const std::string& key, const std::string& val) override {
        std::optional<std::string> evicted;
        
        auto it = key_map_.find(key);
        if (it != key_map_.end()) {
            // Key exists - treat as access (freq++)
            auto node_it = it->second;
            int old_freq = node_it->frequency;
            
            // Remove from old frequency list
            frequency_lists_[old_freq].erase(node_it);
            if (frequency_lists_[old_freq].empty()) {
                frequency_lists_.erase(old_freq);
                if (old_freq == min_frequency_) {
                    min_frequency_ = old_freq + 1;
                }
            }
            
            // Insert into new frequency list
            int new_freq = old_freq + 1;
            frequency_lists_[new_freq].emplace_front(key, val, new_freq);
            key_map_[key] = frequency_lists_[new_freq].begin();
        } else {
            // New key
            if (key_map_.size() >= capacity_) {
                // Evict one key from min_frequency_ list (LRU within same freq)
                auto& min_freq_list = frequency_lists_[min_frequency_];
                auto lru_node = min_freq_list.back();
                evicted = lru_node.key;
                key_map_.erase(evicted.value());
                min_freq_list.pop_back();
                
                if (min_freq_list.empty()) {
                    frequency_lists_.erase(min_frequency_);
                }
            }
            
            // Insert new key with frequency 1
            frequency_lists_[1].emplace_front(key, val, 1);
            key_map_[key] = frequency_lists_[1].begin();
            min_frequency_ = 1;
        }
        
        return evicted;
    }
    
    std::vector<std::pair<std::string, std::string>> snapshot() const override {
        std::vector<std::pair<std::string, std::string>> result;
        result.reserve(key_map_.size());
        
        // Collect all frequencies and sort them in descending order
        std::vector<int> frequencies;
        for (const auto& [freq, _] : frequency_lists_) {
            frequencies.push_back(freq);
        }
        std::sort(frequencies.rbegin(), frequencies.rend());
        
        // Iterate through frequency lists from highest to lowest
        for (int freq : frequencies) {
            const auto& freq_list = frequency_lists_.at(freq);
            for (const auto& node : freq_list) {
                result.emplace_back(node.key, node.value);
            }
        }
        
        return result;
    }
    
    void metaForUI(Step& s) const override {
        for (const auto& [key, node_it] : key_map_) {
            s.freq[key] = node_it->frequency;
        }
    }
    
    bool isCacheHit(const std::string& key) const override {
        return key_map_.find(key) != key_map_.end();
    }
};

} // namespace cachesim
