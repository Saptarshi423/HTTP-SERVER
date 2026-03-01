#include "Router.h"

// Fulfills the "addRoute" contract
void Router::addRoute(const std::string& method, const std::string& path, RouteHandler handler) {
    routes.push_back({ method, path, handler });
}

// Fulfills the "handleRequest" contract
HttpResponse Router::handleRequest(const HttpRequest& request) {
    for (const auto& route : routes) {
        if (route.method == request.getMethod() && route.path == request.getPath()) {
            return route.handler(request);
        }
    }

    // 404 Not Found logic
    HttpResponse response;
    response.setStatus(404, "Not Found");
    response.setJson(R"({"status": "error", "message": "Route not found"})");
    return response;
}