#pragma once

#include "../include/types.hpp"
#include <list>
#include <unordered_map>

namespace cachesim {

class ARCPolicy : public IPolicy {
private:
    size_t capacity_;
    int p_; // target size for T1
    
    // Main cache lists
    std::list<std::string> T1_; // recency list
    std::list<std::string> T2_; // frequency list
    
    // Ghost lists (keys only, no values)
    std::list<std::string> B1_; // ghost recency list
    std::list<std::string> B2_; // ghost frequency list
    
    // Value storage and iterators
    std::unordered_map<std::string, std::string> values_;
    std::unordered_map<std::string, std::list<std::string>::iterator> T1_iterators_;
    std::unordered_map<std::string, std::list<std::string>::iterator> T2_iterators_;
    std::unordered_map<std::string, std::list<std::string>::iterator> B1_iterators_;
    std::unordered_map<std::string, std::list<std::string>::iterator> B2_iterators_;

public:
    explicit ARCPolicy(size_t capacity) : capacity_(capacity), p_(0) {}
    
    bool get(const std::string& key, std::string& outVal) override {
        // Check T1
        auto t1_it = T1_iterators_.find(key);
        if (t1_it != T1_iterators_.end()) {
            // Hit in T1 - move to T2
            T1_.erase(t1_it->second);
            T1_iterators_.erase(t1_it);
            
            T2_.push_front(key);
            T2_iterators_[key] = T2_.begin();
            
            outVal = values_[key];
            return true;
        }
        
        // Check T2
        auto t2_it = T2_iterators_.find(key);
        if (t2_it != T2_iterators_.end()) {
            // Hit in T2 - move to front
            T2_.erase(t2_it->second);
            T2_.push_front(key);
            t2_it->second = T2_.begin();
            
            outVal = values_[key];
            return true;
        }
        
        // Check B1
        auto b1_it = B1_iterators_.find(key);
        if (b1_it != B1_iterators_.end()) {
            // Hit in B1 - increase p (toward recency)
            p_ = std::min(p_ + 1, static_cast<int>(capacity_));
            
            // Move from B1 to T2
            B1_.erase(b1_it->second);
            B1_iterators_.erase(b1_it);
            
            T2_.push_front(key);
            T2_iterators_[key] = T2_.begin();
            
            outVal = values_[key];
            return true;
        }
        
        // Check B2
        auto b2_it = B2_iterators_.find(key);
        if (b2_it != B2_iterators_.end()) {
            // Hit in B2 - decrease p (toward frequency)
            p_ = std::max(p_ - 1, 0);
            
            // Move from B2 to T2
            B2_.erase(b2_it->second);
            B2_iterators_.erase(b2_it);
            
            T2_.push_front(key);
            T2_iterators_[key] = T2_.begin();
            
            outVal = values_[key];
            return true;
        }
        
        return false; // miss
    }
    
    std::optional<std::string> put(const std::string& key, const std::string& val) override {
        std::optional<std::string> evicted;
        
        // Check if key already exists in main cache
        if (T1_iterators_.find(key) != T1_iterators_.end() || 
            T2_iterators_.find(key) != T2_iterators_.end()) {
            // Update existing value and treat as access
            values_[key] = val;
            std::string dummy; // Temporary variable for get() call
            get(key, dummy); // This will move it to T2
            return evicted;
        }
        
        // New key - check if we need to evict
        if (T1_.size() + T2_.size() >= capacity_) {
            if (static_cast<int>(T1_.size()) > p_) {
                // Evict from T1
                std::string evicted_key = T1_.back();
                T1_.pop_back();
                T1_iterators_.erase(evicted_key);
                
                // Move to B1
                B1_.push_front(evicted_key);
                B1_iterators_[evicted_key] = B1_.begin();
                
                evicted = evicted_key;
            } else {
                // Evict from T2
                std::string evicted_key = T2_.back();
                T2_.pop_back();
                T2_iterators_.erase(evicted_key);
                
                // Move to B2
                B2_.push_front(evicted_key);
                B2_iterators_[evicted_key] = B2_.begin();
                
                evicted = evicted_key;
            }
        }
        
        // Clean up ghost lists if they exceed capacity
        while (T1_.size() + T2_.size() + B1_.size() + B2_.size() > 2 * capacity_) {
            if (B1_.size() > 0) {
                std::string key_to_remove = B1_.back();
                B1_.pop_back();
                B1_iterators_.erase(key_to_remove);
                values_.erase(key_to_remove);
            } else if (B2_.size() > 0) {
                std::string key_to_remove = B2_.back();
                B2_.pop_back();
                B2_iterators_.erase(key_to_remove);
                values_.erase(key_to_remove);
            }
        }
        
        // Insert new key into T1
        T1_.push_front(key);
        T1_iterators_[key] = T1_.begin();
        values_[key] = val;
        
        return evicted;
    }
    
    std::vector<std::pair<std::string, std::string>> snapshot() const override {
        std::vector<std::pair<std::string, std::string>> result;
        result.reserve(T1_.size() + T2_.size());
        
        // T2 first (frequency), then T1 (recency)
        for (const auto& key : T2_) {
            result.emplace_back(key, values_.at(key));
        }
        for (const auto& key : T1_) {
            result.emplace_back(key, values_.at(key));
        }
        
        return result;
    }
    
    void metaForUI(Step& s) const override {
        ArcMeta meta;
        meta.p = p_;
        
        for (const auto& key : T1_) meta.T1.push_back(key);
        for (const auto& key : T2_) meta.T2.push_back(key);
        for (const auto& key : B1_) meta.B1.push_back(key);
        for (const auto& key : B2_) meta.B2.push_back(key);
        
        s.arc = meta;
    }
    
    bool isCacheHit(const std::string& key) const override {
        // Only return true if the key is in the main cache (T1 or T2)
        return T1_iterators_.find(key) != T1_iterators_.end() || 
               T2_iterators_.find(key) != T2_iterators_.end();
    }
};

} // namespace cachesim
