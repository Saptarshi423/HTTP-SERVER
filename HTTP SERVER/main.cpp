#include <string>
#include <iostream>
#include "HttpResponse.h"
#include "HttpRequest.h"
#include "Router.h"
#include "HTTP SERVER.h"
#include "FileStorage.h"
#include "Utility.h"


int main()
{
    // Create server
    HttpServer server("127.0.0.1", 8000);

    // Create file storage
    FileStorage storage("post_data.txt");

    // Get router
    Router& router = server.getRouter();

    // Define routes

    // GET / - Home page
    router.addRoute("GET", "/", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse res;
        std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>OOP HTTP Server</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; }
        h1 { color: #333; }
        form { background: #f4f4f4; padding: 20px; border-radius: 5px; margin: 20px 0; }
        input, textarea { width: 100%; padding: 10px; margin: 10px 0; box-sizing: border-box; }
        button { background: #007bff; color: white; padding: 10px 20px; border: none; cursor: pointer; }
        a { color: #007bff; text-decoration: none; margin-right: 15px; }
    </style>
</head>
<body>
    <h1>Object-Oriented HTTP Server</h1>
    <p>This server is built with proper OOP design!</p>
    
    <h2>Submit Data</h2>
    <form action="/submit" method="POST">
        <input type="text" name="name" placeholder="Your name" required>
        <textarea name="message" rows="4" placeholder="Your message" required></textarea>
        <button type="submit">Submit</button>
    </form>
    
    <p>
        <a href="/data">View Saved Data (JSON)</a>
        <a href="/health">Health Check</a>
    </p>
</body>
</html>
)";
        res.setBody(html);
        return res;
        });

    // POST /submit - Save data
    router.addRoute("POST", "/data", [&storage](const HttpRequest& req) -> HttpResponse {
        HttpResponse res;

        std::string body = req.getBody();
        bool saved = storage.save(body);

        if (saved) {
            std::string json = R"({
    "status": "success",
    "message": "Data saved successfully",
    "saved_to": "post_data.txt",
    "timestamp": ")" + Utils::getTimestamp() + R"(",
    "data_received": ")" + Utils::escapeJson(body) + R"("
})";
            res.setJson(json);
        }
        else {
            res.setStatus(500, "Internal Server Error");
            res.setJson(R"({"status": "error", "message": "Could not save data"})");
        }

        return res;
        });

    // GET /data - View all saved data
    router.addRoute("GET", "/data", [&storage](const HttpRequest& req) -> HttpResponse {
        HttpResponse res;

        std::string data = storage.readAll();
        std::string json = R"({
    "status": "success",
    "data": ")" + Utils::escapeJson(data) + R"("
})";

        res.setJson(json);
        return res;
        });

    // GET /health - Health check endpoint
    router.addRoute("GET", "/health", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse res;
        res.setJson(R"({"status": "healthy", "server": "OOP HTTP Server"})");
        return res;
        });

    // Initialize and start server
    if (!server.initialize()) {
        std::cout << "Failed to initialize server\n";
        return 1;
    }

    std::cout << "\nAvailable routes:\n";
    std::cout << "  GET  /        - Home page\n";
    std::cout << "  POST /data  - Submit data\n";
    std::cout << "  GET  /data    - View saved data (JSON)\n";
    std::cout << "  GET  /health  - Health check\n\n";

    server.start();

    return 0;
}