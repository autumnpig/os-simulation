#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>

struct FileNode {
    std::string fileName;
    int size;
    std::string content; // 模拟文件内容
    int createdAt;       // 创建时间 (对应全局时间)
};

class StorageManager {
public:
    StorageManager(int capacity = 1024);

    // 创建文件
    bool createFile(const std::string& name, int size);
    
    // 删除文件
    bool deleteFile(const std::string& name);
    
    // 写入/覆盖文件内容
    bool writeFile(const std::string& name, const std::string& content);
    
    // 读取文件内容
    std::string readFile(const std::string& name);
    
    // 列出所有文件
    void listFiles() const;

    // 获取剩余磁盘空间
    int getFreeSpace() const;

    // 获取文件大小
    int getFileSize(const std::string& name) const;

    // 将虚拟磁盘保存到真实的本地文件
    void saveToDisk(const std::string& realFileName) const;

    // 从真实的本地文件加载虚拟磁盘
    void loadFromDisk(const std::string& realFileName);

private:
    int totalCapacity;
    int usedSpace;
    std::map<std::string, FileNode> fileSystem; // 文件名 -> 文件节点
};

#endif // STORAGE_H