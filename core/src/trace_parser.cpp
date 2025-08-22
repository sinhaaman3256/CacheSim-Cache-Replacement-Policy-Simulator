#include "trace_parser.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio> // Added for printf

namespace cachesim {

ParseResult TraceParser::parse(const std::string& traceText) {
    ParseResult result;
    result.success = true;
    
    printf("DEBUG: TraceParser::parse called with traceText: '%s'\n", traceText.c_str());
    printf("DEBUG: traceText length: %zu\n", traceText.length());
    
    // First, replace escaped newlines with actual newlines
    std::string processedText = traceText;
    size_t pos = 0;
    while ((pos = processedText.find("\\n", pos)) != std::string::npos) {
        processedText.replace(pos, 2, "\n");
        pos += 1; // Move past the newline we just inserted
    }
    
    printf("DEBUG: After processing escaped newlines: '%s'\n", processedText.c_str());
    
    // Manual line splitting on actual newlines
    std::vector<std::string> lines;
    size_t start = 0;
    size_t end = processedText.find('\n');
    
    while (end != std::string::npos) {
        std::string line = processedText.substr(start, end - start);
        if (!line.empty()) {
            lines.push_back(line);
        }
        start = end + 1;
        end = processedText.find('\n', start);
    }
    // Add the last line if there's no trailing newline
    if (start < processedText.length()) {
        std::string lastLine = processedText.substr(start);
        if (!lastLine.empty()) {
            lines.push_back(lastLine);
        }
    }
    
    printf("DEBUG: Split into %zu lines\n", lines.size());
    
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        int lineNumber = i + 1;
        
        printf("DEBUG: Processing line %d: '%s'\n", lineNumber, line.c_str());
        
        std::string trimmedLine = trim(line);
        printf("DEBUG: After trim: '%s'\n", trimmedLine.c_str());
        
        if (isEmpty(trimmedLine) || isComment(trimmedLine)) {
            printf("DEBUG: Line %d is empty or comment, skipping\n", lineNumber);
            continue;
        }
        
        try {
            TraceOp op = parseLine(trimmedLine, lineNumber);
            printf("DEBUG: Successfully parsed line %d: %s %s %s\n", 
                   lineNumber, 
                   (op.kind == TraceOp::Kind::GET ? "GET" : "PUT"),
                   op.key.c_str(),
                   op.value.c_str());
            result.operations.push_back(op);
        } catch (const std::exception& e) {
            printf("DEBUG: Error parsing line %d: %s\n", lineNumber, e.what());
            result.errors.push_back("Line " + std::to_string(lineNumber) + ": " + e.what());
            result.success = false;
        }
    }
    
    printf("DEBUG: Parse completed. Success: %s, Operations: %zu, Errors: %zu\n", 
           result.success ? "true" : "false", 
           result.operations.size(), 
           result.errors.size());
    
    return result;
}

TraceOp TraceParser::parseLine(const std::string& line, int lineNumber) {
    (void)lineNumber; // Suppress unused parameter warning
    std::istringstream stream(line);
    std::string op, key, value;
    
    if (!(stream >> op)) {
        throw std::runtime_error("Empty line");
    }
    
    if (op == "GET") {
        if (!(stream >> key)) {
            throw std::runtime_error("GET requires a key");
        }
        
        // Check for extra tokens
        if (stream >> value) {
            throw std::runtime_error("GET should not have a value");
        }
        
        return TraceOp{TraceOp::Kind::GET, key, ""};
        
    } else if (op == "PUT") {
        if (!(stream >> key)) {
            throw std::runtime_error("PUT requires a key");
        }
        
        // Read the rest of the line as value
        std::getline(stream, value);
        value = trim(value);
        
        if (value.empty()) {
            throw std::runtime_error("PUT requires a value");
        }
        
        return TraceOp{TraceOp::Kind::PUT, key, value};
        
    } else {
        throw std::runtime_error("Unknown operation: " + op + " (expected GET or PUT)");
    }
}

std::string TraceParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool TraceParser::isComment(const std::string& line) {
    return !line.empty() && line[0] == '#';
}

bool TraceParser::isEmpty(const std::string& line) {
    return line.empty();
}

} // namespace cachesim
