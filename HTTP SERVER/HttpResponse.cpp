#include "HttpResponse.h"

// Constructor implementation using an initializer list
HttpResponse::HttpResponse()
    : statusCode(200), statusText("OK"), contentType("text/html") {}

void HttpResponse::setStatus(int code, const std::string& text) {
    statusCode = code;
    statusText = text;
}

void HttpResponse::setBody(const std::string& content) {
    body = content;
}

void HttpResponse::setContentType(const std::string& type) {
    contentType = type;
}

void HttpResponse::setJson(const std::string& jsonContent) {
    body = jsonContent;
    contentType = "application/json";
}

std::string HttpResponse::build() const {
    std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    return response;
}