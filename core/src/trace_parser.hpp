#pragma once

#include "../include/types.hpp"
#include <string>
#include <vector>

namespace cachesim {

struct ParseResult {
    std::vector<TraceOp> operations;
    std::vector<std::string> errors;
    bool success;
};

class TraceParser {
public:
    static ParseResult parse(const std::string& traceText);
    
private:
    static TraceOp parseLine(const std::string& line, int lineNumber);
    static std::string trim(const std::string& str);
    static bool isComment(const std::string& line);
    static bool isEmpty(const std::string& line);
};

} // namespace cachesim
