#ifndef PROGRESS_UTILS_HPP
#define PROGRESS_UTILS_HPP

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct FileProgress {
    std::string filename;
    int progress;
};

void saveProgressToJson(const std::string& filename, int progress);

FileProgress readProgressFromJson(const std::string& filenameToFind);

#endif // PROGRESS_UTILS_HPP
