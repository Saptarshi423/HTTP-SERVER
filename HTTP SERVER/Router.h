#pragma once

#include <string>
#include <functional>
#include <vector>
#include "HttpResponse.h"
#include "HttpRequest.h"

class Router {
public:
    // Define the type for route handlers
    using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

private:
    struct Route {
        std::string method;
        std::string path;
        RouteHandler handler;
    };

    std::vector<Route> routes;

public:
    // Declarations: These tell other files what the class can do
    void addRoute(const std::string& method, const std::string& path, RouteHandler handler);
    HttpResponse handleRequest(const HttpRequest& request);
};