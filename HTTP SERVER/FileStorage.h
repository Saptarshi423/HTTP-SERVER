#pragma once
#include <string>
#include <mutex>

class FileStorage {
private:
    std::string filename;
    std::mutex fileMutex;

public:
    FileStorage(const std::string& file);

    bool save(const std::string& data);
    std::string readAll();
};