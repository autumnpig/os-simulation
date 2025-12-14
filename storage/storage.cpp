#include "storage.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <stdexcept>

/**
 * @brief 读取文件内容
 */
std::string Storage::readFile(const std::string& filename)
{
    std::ifstream inFile(filename, std::ios::in);
    if (!inFile.is_open())
    {
        throw std::runtime_error("Failed to open file for reading: " + filename);
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    inFile.close();

    return buffer.str();
}

/**
 * @brief 写入文件内容
 */
void Storage::writeFile(const std::string& filename, const std::string& data)
{
    std::ofstream outFile(filename, std::ios::out | std::ios::trunc);
    if (!outFile.is_open())
    {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    outFile << data;
    outFile.close();
}

/**
 * @brief 删除文件
 */
void Storage::deleteFile(const std::string& filename)
{
    if (std::remove(filename.c_str()) != 0)
    {
        throw std::runtime_error("Failed to delete file: " + filename);
    }
}
