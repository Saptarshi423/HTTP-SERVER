#include "HttpRequest.h"
#include <iostream>

// Constructor implementation
HttpRequest::HttpRequest(const std::string& request) : rawRequest(request) {
    parse();
}

void HttpRequest::parse() {
    if (rawRequest.empty()) return;

    // Parse request line (e.g., "GET / HTTP/1.1")
    size_t firstSpace = rawRequest.find(' ');
    size_t secondSpace = rawRequest.find(' ', firstSpace + 1);

    if (firstSpace != std::string::npos && secondSpace != std::string::npos) {
        method = rawRequest.substr(0, firstSpace);
        path = rawRequest.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    }

    // Parse body (everything after \r\n\r\n)
    size_t bodyPos = rawRequest.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        body = rawRequest.substr(bodyPos + 4);
    }
}

std::string HttpRequest::getMethod() const { return method; }
std::string HttpRequest::getPath() const { return path; }
std::string HttpRequest::getBody() const { return body; }
std::string HttpRequest::getRaw() const { return rawRequest; }

void HttpRequest::print() const {
    std::cout << "Method: " << method << ", Path: " << path << "\n";
    if (!body.empty()) {
        std::cout << "Body: " << body << "\n";
    }
}