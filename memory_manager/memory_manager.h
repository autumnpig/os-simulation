// autumnpig/os-simulation/.../memory_manager/memory_manager.h
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <cstdint>
#include <iostream>

class MemoryManager {
public:
    // 构造函数：初始化总大小、页大小和物理块上限
    MemoryManager(int totalSize = 1024, int pageSize = 32, int maxFrames = 16);

    // 连续分区管理
    int* allocateMemory(int size);
    void freeMemory(int* ptr);

    // 虚拟存储管理：模拟访问页面
    void accessPage(int page, bool write = false);

private:
    struct Block {
        int start;
        int size;
    };

    struct PageTableEntry {
        int frame;
        bool present;
        bool inSwap;
        bool fileBacked;
        bool dirty;
    };

    // 成员变量
    std::vector<Block> freeList;
    std::unordered_map<int, Block> usedBlocks;
    std::unordered_map<int, PageTableEntry> pageTable;
    std::list<int> lruList;
    std::unordered_set<int> swapArea;
    std::unordered_set<int> fileArea;

    int totalSize;
    int pageSize;
    int maxFrames;

    void swapIn(int page);
    void swapOut(int page);
};

#endif