#pragma once

#include "../include/types.hpp"
#include <memory>
#include <vector>

namespace cachesim {

class Simulator {
public:
    SimResult run(const std::vector<TraceOp>& ops, IPolicy& policy, const SimConfig& cfg);
    
private:
    Step createStep(int index, const TraceOp& op, bool hit, 
                   const std::optional<std::string>& evicted, 
                   const IPolicy& policy);
};

} // namespace cachesim
