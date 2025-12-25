// memory_manager/memory_manager.h
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <cstdint>
#include <iostream>
#include <algorithm> // for sort
#include <iomanip>   // for setw

class MemoryManager {
public:
    MemoryManager(int totalSize = 1024, int pageSize = 32, int maxFrames = 16); // maxFrames 默认改小一点方便演示，比如 4

    // 连续分区管理
    int* allocateMemory(int size);
    void freeMemory(int* ptr);

    // 虚拟存储管理：模拟访问页面
    void accessPage(int page, bool write = false);

    // 【新增】打印内存状态（分区情况 + 分页情况）
    void printStatus() const;

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

    std::vector<Block> freeList;
    std::unordered_map<int, Block> usedBlocks;
    std::unordered_map<int, PageTableEntry> pageTable;
    std::list<int> lruList; // 模拟物理内存帧的 LRU 队列
    std::unordered_set<int> swapArea;
    std::unordered_set<int> fileArea;

    int totalSize;
    int pageSize;
    int maxFrames;

    void swapIn(int page);
    void swapOut(int page);
};

#endif