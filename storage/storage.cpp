#include "storage.h"
#include <iomanip>
#include <cstring> // for memset

StorageManager::StorageManager(int capacity) : totalCapacity(capacity) {
    // 初始化位图，全部空闲
    for(int i=0; i<TOTAL_BLOCKS; ++i) blockBitmap[i] = false;
}

// 【核心逻辑】尝试分配物理块
bool StorageManager::allocateBlocks(int size, std::vector<int>& outBlocks) {
    int blocksNeeded = std::ceil((double)size / BLOCK_SIZE);
    std::vector<int> tempBlocks;

    // 扫描位图寻找空闲块
    for (int i = 0; i < TOTAL_BLOCKS; ++i) {
        if (!blockBitmap[i]) {
            tempBlocks.push_back(i);
            if (tempBlocks.size() == blocksNeeded) break;
        }
    }

    if (tempBlocks.size() < blocksNeeded) {
        return false; // 空间不足
    }

    // 真正分配
    for (int blockIndex : tempBlocks) {
        blockBitmap[blockIndex] = true; // 标记占用
    }
    outBlocks = tempBlocks;
    return true;
}

// 【核心逻辑】释放物理块
void StorageManager::freeBlocks(const std::vector<int>& blocks) {
    for (int blockIndex : blocks) {
        if (blockIndex >= 0 && blockIndex < TOTAL_BLOCKS) {
            blockBitmap[blockIndex] = false; // 标记为空闲
        }
    }
}

bool StorageManager::createFile(const std::string& name, int size) {
    if (fileSystem.find(name) != fileSystem.end()) {
        std::cout << "[Storage] Error: File '" << name << "' already exists.\n";
        return false;
    }
    
    // 尝试分配块
    std::vector<int> blocks;
    if (!allocateBlocks(size, blocks)) {
        std::cout << "[Storage] Error: Not enough disk blocks.\n";
        return false;
    }

    FileNode newNode;
    newNode.fileName = name;
    newNode.size = size;
    newNode.content = ""; 
    newNode.createdAt = 0;
    newNode.blockIndices = blocks; // 记录占用的块
    
    fileSystem[name] = newNode;
    
    std::cout << "[Storage] File '" << name << "' created. Allocated " 
              << blocks.size() << " blocks (Indices: ";
    for(int b : blocks) std::cout << b << " ";
    std::cout << ").\n";
    
    return true;
}

bool StorageManager::deleteFile(const std::string& name) {
    auto it = fileSystem.find(name);
    if (it != fileSystem.end()) {
        // 释放块
        freeBlocks(it->second.blockIndices);
        fileSystem.erase(it);
        std::cout << "[Storage] File '" << name << "' deleted & blocks freed.\n";
        return true;
    }
    std::cout << "[Storage] Error: File '" << name << "' not found.\n";
    return false;
}

bool StorageManager::writeFile(const std::string& name, const std::string& content) {
    if (fileSystem.find(name) == fileSystem.end()) {
        std::cout << "[Storage] Error: File '" << name << "' not found.\n";
        return false;
    }
    
    FileNode& node = fileSystem[name];
    if (content.length() > node.size) {
        std::cout << "[Storage] Error: Content exceeds file size (" 
                  << node.size << " bytes). Re-create file with larger size.\n";
        return false;
    }

    node.content = content;
    std::cout << "[Storage] Wrote to '" << name << "'.\n";
    return true;
}

std::string StorageManager::readFile(const std::string& name) {
    if (fileSystem.find(name) != fileSystem.end()) {
        return fileSystem[name].content;
    }
    return "";
}

void StorageManager::listFiles() const {
    std::cout << "\n--- File System (Block Size: " << BLOCK_SIZE << ") ---\n";
    std::cout << std::left << std::setw(15) << "Name" 
              << std::setw(8) << "Size" 
              << std::setw(8) << "Blocks"
              << "Block Indices\n";
    std::cout << "--------------------------------------------------------\n";
    for (const auto& pair : fileSystem) {
        const auto& node = pair.second;
        std::cout << std::left << std::setw(15) << pair.first 
                  << std::setw(8) << node.size 
                  << std::setw(8) << node.blockIndices.size() << "[ ";
        for(int b : node.blockIndices) std::cout << b << " ";
        std::cout << "]\n";
    }
    printDiskStatus();
}

int StorageManager::getFreeSpace() const {
    int freeBlocks = 0;
    for(int i=0; i<TOTAL_BLOCKS; ++i) if(!blockBitmap[i]) freeBlocks++;
    return freeBlocks * BLOCK_SIZE;
}

int StorageManager::getFileSize(const std::string& name) const {
    if (fileSystem.find(name) != fileSystem.end()) return fileSystem.at(name).size;
    return -1;
}

// 显示磁盘位图
void StorageManager::printDiskStatus() const {
    std::cout << "[Disk Bitmap] (0=Free, 1=Used): ";
    for(int i=0; i<TOTAL_BLOCKS; ++i) {
        std::cout << (blockBitmap[i] ? "1" : "0");
    }
    std::cout << "\n--------------------------------------------------------\n";
}

void StorageManager::saveToDisk(const std::string& realFileName) const {
    std::ofstream outFile(realFileName);
    if (!outFile.is_open()) return;
    // 简单保存：文件数量
    outFile << fileSystem.size() << "\n";
    for (const auto& pair : fileSystem) {
        const FileNode& node = pair.second;
        // 格式：Name Size Content
        outFile << node.fileName << " " << node.size << " " << (node.content.empty() ? "EMPTY" : node.content) << "\n";
    }
    outFile.close();
}

void StorageManager::loadFromDisk(const std::string& realFileName) {
    std::ifstream inFile(realFileName);
    if (!inFile.is_open()) return;
    
    // 重置
    fileSystem.clear();
    for(int i=0; i<TOTAL_BLOCKS; ++i) blockBitmap[i] = false;

    int count;
    inFile >> count;
    std::string name, content;
    int size;
    for (int i = 0; i < count; ++i) {
        inFile >> name >> size >> content;
        if(content == "EMPTY") content = "";
        createFile(name, size); // 重新调用 create 以分配块
        fileSystem[name].content = content;
    }
    inFile.close();
}