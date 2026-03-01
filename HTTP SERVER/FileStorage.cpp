#include "FileStorage.h"
#include "Utility.h"
#include <fstream>
#include <sstream>
#include <iostream>

FileStorage::FileStorage(const std::string& file) : filename(file) {}

bool FileStorage::save(const std::string& data) {
    std::unique_lock<std::mutex> lock(fileMutex);
    // Open in append mode
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) {
        std::cout << "Could not open file for writing: " << filename << "\n";
        return false;
    }

    file << "=== Entry at " << Utils::getTimestamp() << " ===\n";
    file << data << "\n";
    file << "================================\n\n";
    file.close();

    std::cout << "Data saved to " << filename << "\n";
    return true;
}

std::string FileStorage::readAll() {
    std::unique_lock<std::mutex> lock(fileMutex);
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "No data saved yet.";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}