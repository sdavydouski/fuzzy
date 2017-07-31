#pragma once

#include <string>

namespace fuzzy::utils {

class FileManager {
public:
    static std::string readText(const std::string& path);
private:
    FileManager() {};
    ~FileManager() {};
};

}
