#include "Utility.h" // Links this logic to the blueprint
#include <ctime>

std::string Utils::getTimestamp() {
    time_t now = time(0);
    char buf[80];
    struct tm timeinfo;

    // Logic for generating the time
    localtime_s(&timeinfo, &now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);

    return std::string(buf);
}

std::string Utils::escapeJson(const std::string& input) {
    std::string escaped;
    // Logic for cleaning the string
    for (char c : input) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else if (c == '\\') escaped += "\\\\";
        else escaped += c;
    }
    return escaped;
}