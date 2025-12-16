#include "storage.h"
#include <iomanip>

StorageManager::StorageManager(int capacity) : totalCapacity(capacity), usedSpace(0) {}

bool StorageManager::createFile(const std::string& name, int size) {
    if (fileSystem.find(name) != fileSystem.end()) {
        std::cout << "[Storage] Error: File '" << name << "' already exists." << std::endl;
        return false;
    }
    
    if (usedSpace + size > totalCapacity) {
        std::cout << "[Storage] Error: Not enough disk space. (Free: " << getFreeSpace() << ")" << std::endl;
        return false;
    }

    FileNode newNode;
    newNode.fileName = name;
    newNode.size = size;
    newNode.content = ""; // 初始为空
    newNode.createdAt = 0; // 这里的简单实现暂不获取 Scheduler 时间，可后续优化
    
    fileSystem[name] = newNode;
    usedSpace += size;
    std::cout << "[Storage] File '" << name << "' created (Size: " << size << ")." << std::endl;
    return true;
}

bool StorageManager::deleteFile(const std::string& name) {
    auto it = fileSystem.find(name);
    if (it != fileSystem.end()) {
        usedSpace -= it->second.size;
        fileSystem.erase(it);
        std::cout << "[Storage] File '" << name << "' deleted." << std::endl;
        return true;
    }
    std::cout << "[Storage] Error: File '" << name << "' not found." << std::endl;
    return false;
}

bool StorageManager::writeFile(const std::string& name, const std::string& content) {
    if (fileSystem.find(name) == fileSystem.end()) {
        std::cout << "[Storage] Error: File '" << name << "' does not exist." << std::endl;
        return false;
    }
    fileSystem[name].content = content;
    std::cout << "[Storage] Wrote " << content.length() << " bytes to '" << name << "'." << std::endl;
    return true;
}

std::string StorageManager::readFile(const std::string& name) {
    if (fileSystem.find(name) != fileSystem.end()) {
        return fileSystem[name].content;
    }
    std::cout << "[Storage] Error: File '" << name << "' not found." << std::endl;
    return "";
}

void StorageManager::listFiles() const {
    std::cout << "\n--- File System (Used: " << usedSpace << "/" << totalCapacity << ") ---\n";
    std::cout << std::left << std::setw(15) << "Name" << std::setw(10) << "Size" << "Content Preview" << std::endl;
    std::cout << "--------------------------------------------\n";
    for (const auto& pair : fileSystem) {
        std::string preview = pair.second.content.substr(0, 15);
        if (pair.second.content.length() > 15) preview += "...";
        
        std::cout << std::left << std::setw(15) << pair.first 
                  << std::setw(10) << pair.second.size 
                  << preview << std::endl;
    }
    std::cout << "--------------------------------------------\n";
}

int StorageManager::getFreeSpace() const {
    return totalCapacity - usedSpace;
}

int StorageManager::getFileSize(const std::string& name) const {
    if (fileSystem.find(name) != fileSystem.end()) {
        return fileSystem.at(name).size;
    }
    return -1; // -1 表示文件不存在
}

void StorageManager::saveToDisk(const std::string& realFileName) const {
    std::ofstream outFile(realFileName);
    if (!outFile.is_open()) {
        std::cout << "[Storage] Error: Could not save to disk file '" << realFileName << "'.\n";
        return;
    }

    // 简单格式：总容量 已用空间 文件数量
    // 然后每行一个文件：文件名 大小 内容
    outFile << totalCapacity << " " << usedSpace << " " << fileSystem.size() << "\n";

    for (const auto& pair : fileSystem) {
        const FileNode& node = pair.second;
        // 注意：为了简化，这里假设文件名和内容不包含空格。
        // 如果要支持空格，需要更复杂的序列化方法（如长度前缀法或JSON）。
        // 这里我们用简单的 "文件名 大小 内容" 格式
        outFile << node.fileName << " " << node.size << " " << node.content << "\n";
    }

    outFile.close();
    std::cout << "[Storage] System saved to '" << realFileName << "'.\n";
}

void StorageManager::loadFromDisk(const std::string& realFileName) {
    std::ifstream inFile(realFileName);
    if (!inFile.is_open()) {
        std::cout << "[Storage] No previous disk image found. Starting fresh.\n";
        return;
    }

    int fileCount;
    // 读取头信息
    inFile >> totalCapacity >> usedSpace >> fileCount;

    fileSystem.clear(); // 清空当前内存里的数据

    std::string name, content;
    int size;
    for (int i = 0; i < fileCount; ++i) {
        inFile >> name >> size >> content;
        
        FileNode node;
        node.fileName = name;
        node.size = size;
        node.content = content;
        node.createdAt = 0; // 暂不恢复时间
        
        fileSystem[name] = node;
    }

    inFile.close();
    std::cout << "[Storage] System loaded from '" << realFileName << "'. (" << fileCount << " files)\n";
}