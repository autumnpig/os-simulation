#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

// 定义磁盘块大小（例如每块 32 字节）
const int BLOCK_SIZE = 32;
// 假设总容量 1024 字节 -> 32 个块
const int TOTAL_BLOCKS = 1024 / BLOCK_SIZE;

struct FileNode {
    std::string fileName;
    int size;
    std::string content; // 实际内容（为了简化读写，仍然保留这个，但逻辑上它分散在 blockIndices 里）
    int createdAt;       
    
    // 索引表：记录该文件占用了哪些物理块
    std::vector<int> blockIndices; 
};

class StorageManager {
public:
    StorageManager(int capacity = 1024);

    bool createFile(const std::string& name, int size);
    bool deleteFile(const std::string& name);
    bool writeFile(const std::string& name, const std::string& content);
    std::string readFile(const std::string& name);
    void listFiles() const;
    int getFreeSpace() const;
    int getFileSize(const std::string& name) const;

    void saveToDisk(const std::string& realFileName) const;
    void loadFromDisk(const std::string& realFileName);

    // 【新增】打印磁盘位图状态（用于展示块分配原理）
    void printDiskStatus() const;

private:
    int totalCapacity;
    std::map<std::string, FileNode> fileSystem; 

    // 【新增】位示图：记录哪些块被占用了 (true=占用, false=空闲)
    bool blockBitmap[TOTAL_BLOCKS];

    // 辅助：分配块
    bool allocateBlocks(int size, std::vector<int>& outBlocks);
    // 辅助：释放块
    void freeBlocks(const std::vector<int>& blocks);
};

#endif // STORAGE_H