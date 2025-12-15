#include "memory_manager.h"
#include <iostream>

int MemoryManager::memoryPool[1024] = {0};
bool MemoryManager::memorySwappedIn[1024] = {false};
std::map<int*, int> MemoryManager::allocatedSizes; // 定义静态成员

int* MemoryManager::allocateMemory(int size) {
    for (int i = 0; i < 1024 - size; ++i) {
        bool available = true;
        for (int j = 0; j < size; ++j) {
            if (memorySwappedIn[i + j]) {
                available = false;
                break;
            }
        }
        if (available) {
            for (int j = 0; j < size; ++j) {
                memorySwappedIn[i + j] = true;
            }
            // 新增：记录这笔分配的大小
            int* ptr = &memoryPool[i];
            allocatedSizes[ptr] = size;
            
            std::cout << "[Memory] Allocated " << size << " units at index " << i << std::endl;
            return ptr;
        }
    }
    std::cout << "[Memory] Allocation failed: Not enough space." << std::endl;
    return nullptr;
}

void MemoryManager::freeMemory(int* ptr) {
    // 新增：查账本，看这个指针当初申请了多大
    if (allocatedSizes.find(ptr) != allocatedSizes.end()) {
        int size = allocatedSizes[ptr]; //以此为依据释放
        int index = ptr - memoryPool;
        
        for (int i = 0; i < size; ++i) {
            memorySwappedIn[index + i] = false;
        }
        
        allocatedSizes.erase(ptr); // 销毁账目
        std::cout << "[Memory] Freed " << size << " units at index " << index << std::endl;
    } else {
        std::cout << "[Memory] Error: Invalid pointer or double free." << std::endl;
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
