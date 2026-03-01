#pragma once
#include <string>
#include <map>

class HttpRequest {
private:
    std::string rawRequest;
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;

    // Internal helper to process the raw string
    void parse();

public:
    // Constructor
    HttpRequest(const std::string& request);

    // Getters
    std::string getMethod() const;
    std::string getPath() const;
    std::string getBody() const;
    std::string getRaw() const;

    // Utility
    void print() const;
};