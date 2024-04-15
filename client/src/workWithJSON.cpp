#include "../include/workWithJSON.hpp"

// Функция для записи данных о прогрессе в JSON-файл
void saveProgressToJson(const std::string& filename, int progress) {
    json jsonData;

    // Открываем или создаем файл JSON для дозаписи
    std::ifstream infile("progress.json");
    if (infile.good()) {
        infile >> jsonData;
    }
    infile.close();

    bool updated = false;

    // Обходим массив JSON и проверяем, есть ли такой файл уже в данных
    for (auto& item : jsonData) {
        if (item["filename"] == filename) {
            item["progress"] = progress; // Обновляем прогресс, если файл уже существует
            updated = true;
            break;
        }
    }

    // Если файл не был обновлен, добавляем новую запись
    if (!updated) {
        jsonData.push_back({{"filename", filename}, {"progress", progress}});
    }

    // Записываем данные обратно в файл
    std::ofstream file("progress.json");
    if (file.is_open()) {
        file << std::setw(4) << jsonData << std::endl; // Форматируем с отступами для читаемости
        file.close();
    } else {
        std::cerr << "Failed to open progress.json for writing." << std::endl;
    }
}

// Функция для считывания данных о прогрессе из JSON-файла
FileProgress readProgressFromJson(const std::string& filenameToFind) {
    std::vector<FileProgress> progressList;

    // Открываем файл JSON для чтения
    std::ifstream file("progress.json");
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open progress.json for reading");
    }

    // Считываем JSON из файла
    json jsonData;
    file >> jsonData;

    // Закрываем файл
    file.close();

    // Обходим массив JSON и добавляем данные в вектор прогресса
    for (const auto& item : jsonData) {
        FileProgress progress;
        progress.filename = item["filename"].get<std::string>();
        progress.progress = item["progress"].get<int>();
        if (progress.filename == filenameToFind) {
            return progress;
        }
    }
    
    FileProgress progress;
    progress.filename = filenameToFind;
    progress.progress = 0;
    
    return progress;
}
