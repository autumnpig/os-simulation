#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

// MemoryManager 类：负责内存的分配、释放、换入和换出操作
class MemoryManager {
public:
    // 分配内存块
    static int* allocateMemory(int size);

    // 释放内存块
    static void freeMemory(int* ptr);

    // 内存换入操作
    static void swapIn(int page);

    // 内存换出操作
    static void swapOut(int page);

private:
    // 模拟内存池
    static int memoryPool[1024];

    // 模拟内存换入换出操作
    static bool memorySwappedIn[1024];
};

#endif // MEMORY_MANAGER_H
