#pragma once
#include <string>

class HttpResponse {
private:
    int statusCode;
    std::string statusText;
    std::string body;
    std::string contentType;

public:
    // Constructor
    HttpResponse();

    // Method Declarations
    void setStatus(int code, const std::string& text);
    void setBody(const std::string& content);
    void setContentType(const std::string& type);
    void setJson(const std::string& jsonContent);
    std::string build() const;
};