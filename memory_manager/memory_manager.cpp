#include "memory_manager.h"
#include <iostream>

// 模拟内存池，大小为 1024 个整数
int MemoryManager::memoryPool[1024] = {0};

// 记录哪些内存页已被换入
bool MemoryManager::memorySwappedIn[1024] = {false};

// 分配内存块
int* MemoryManager::allocateMemory(int size) {
    for (int i = 0; i < 1024 - size; ++i) {
        bool available = true;
        // 查找连续的内存空间
        for (int j = 0; j < size; ++j) {
            if (memorySwappedIn[i + j]) {
                available = false;
                break;
            }
        }
        // 找到可用空间，进行分配
        if (available) {
            for (int j = 0; j < size; ++j) {
                memorySwappedIn[i + j] = true;
            }
            std::cout << "Memory allocated at position: " << i << std::endl;
            return &memoryPool[i];
        }
    }
    std::cout << "Memory allocation failed: Not enough space." << std::endl;
    return nullptr;
}

// 释放内存块
void MemoryManager::freeMemory(int* ptr) {
    int index = ptr - memoryPool;  // 计算内存块的起始位置
    if (index >= 0 && index < 1024) {
        memorySwappedIn[index] = false;
        std::cout << "Memory freed at position: " << index << std::endl;
    } else {
        std::cout << "Invalid pointer. Cannot free memory." << std::endl;
    }
}

// 内存换入操作
void MemoryManager::swapIn(int page) {
    if (page >= 0 && page < 1024) {
        memorySwappedIn[page] = true;
        std::cout << "Memory swapped in at page: " << page << std::endl;
    } else {
        std::cout << "Invalid page. Cannot swap in." << std::endl;
    }
}

// 内存换出操作
void MemoryManager::swapOut(int page) {
    if (page >= 0 && page < 1024) {
        memorySwappedIn[page] = false;
        std::cout << "Memory swapped out at page: " << page << std::endl;
    } else {
        std::cout << "Invalid page. Cannot swap out." << std::endl;
    }
}
