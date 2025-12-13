#include <iostream>
#include "memory_manager.h"

int main() {
    // 测试内存分配
    int* ptr1 = MemoryManager::allocateMemory(5);
    int* ptr2 = MemoryManager::allocateMemory(10);

    // 测试内存释放
    if (ptr1) {
        MemoryManager::freeMemory(ptr1);
    }

    // 测试内存换入换出
    MemoryManager::swapIn(20);
    MemoryManager::swapOut(20);

    // 再次测试内存分配
    int* ptr3 = MemoryManager::allocateMemory(3);
    
    if (ptr2) {
        MemoryManager::freeMemory(ptr2);
    }

    if (ptr3) {
        MemoryManager::freeMemory(ptr3);
    }

    return 0;
}
