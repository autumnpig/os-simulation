#ifndef STORAGE_H
#define STORAGE_H

#include <string>

/**
 * @class Storage
 * @brief 文件存储模块，提供文件读写与删除功能
 */
class Storage
{
public:
    /**
     * @brief 读取文件内容
     * @param filename 文件名（含路径）
     * @return 文件内容字符串
     */
    std::string readFile(const std::string& filename);

    /**
     * @brief 写入文件内容（覆盖写）
     * @param filename 文件名（含路径）
     * @param data 写入的数据
     */
    void writeFile(const std::string& filename, const std::string& data);

    /**
     * @brief 删除指定文件
     * @param filename 文件名（含路径）
     */
    void deleteFile(const std::string& filename);
};

#endif // STORAGE_H
