#pragma once
#include <string>

class FileStorage {
private:
    std::string filename;

public:
    FileStorage(const std::string& file);

    bool save(const std::string& data);
    std::string readAll();
};