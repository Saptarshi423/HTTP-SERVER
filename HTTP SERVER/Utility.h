#pragma once  // Prevents the file from being loaded twice
#include <string>

class Utils {
public:
    // These are just "promises" that the code exists elsewhere
    static std::string getTimestamp();
    static std::string escapeJson(const std::string& input);
};