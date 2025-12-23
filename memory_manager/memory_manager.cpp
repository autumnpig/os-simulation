#include "memory_manager.h"
#include <iostream>
#include <algorithm>

/* ================= 构造函数 ================= */

MemoryManager::MemoryManager(int totalSize, int pageSize, int maxFrames)
    : totalSize(totalSize),
      pageSize(pageSize),
      maxFrames(maxFrames) {

    freeList.push_back({1, totalSize});

    // 逻辑文件区：0~9 页
    for (int i = 0; i < 10; ++i) {
        fileArea.insert(i);
    }
}

/* ================= 连续分区管理 ================= */

int* MemoryManager::allocateMemory(int size) {
    for (auto it = freeList.begin(); it != freeList.end(); ++it) {
        if (it->size >= size) {
            int addr = it->start;
            usedBlocks.emplace(addr, Block{addr, size});

            if (it->size == size) {
                freeList.erase(it);
            } else {
                it->start += size;
                it->size -= size;
            }

            std::cout << "Allocate memory at " << addr
                      << ", size=" << size << std::endl;

            // 使用 intptr_t 作为安全中转
            return reinterpret_cast<int*>(static_cast<intptr_t>(addr));
        }
    }
    return nullptr;
}

void MemoryManager::freeMemory(int* ptr) {
    if (ptr == nullptr) return;

    int addr = static_cast<int>(reinterpret_cast<intptr_t>(ptr));

    auto it = usedBlocks.find(addr);
    if (it == usedBlocks.end()) return;

    freeList.push_back(it->second);
    usedBlocks.erase(it);

    std::sort(freeList.begin(), freeList.end(),
              [](const Block& a, const Block& b) {
                  return a.start < b.start;
              });

    for (size_t i = 0; i + 1 < freeList.size();) {
        if (freeList[i].start + freeList[i].size == freeList[i + 1].start) {
            freeList[i].size += freeList[i + 1].size;
            freeList.erase(freeList.begin() + static_cast<long>(i + 1));
        } else {
            ++i;
        }
    }

    std::cout << "Free memory at " << addr << std::endl;
}

/* ================= Swap / 文件区 ================= */

void MemoryManager::swapOut(int page) {
    PageTableEntry& pte = pageTable.at(page);

    if (pte.fileBacked) {
        if (pte.dirty) {
            std::cout << "Write back dirty file page " << page << std::endl;
        } else {
            std::cout << "Discard clean file page " << page << std::endl;
        }
    } else {
        std::cout << "Swap out anonymous page " << page << std::endl;
        swapArea.insert(page);
        pte.inSwap = true;
    }

    pte.present = false;
    pte.dirty = false;
    lruList.remove(page);
}

void MemoryManager::swapIn(int page) {
    if (lruList.size() >= static_cast<size_t>(maxFrames)) {
        int victim = lruList.back();
        lruList.pop_back();
        swapOut(victim);
    }

    PageTableEntry& pte = pageTable.at(page);
    pte.frame = static_cast<int>(lruList.size());
    pte.present = true;
    pte.inSwap = false;

    if (pte.fileBacked) {
        std::cout << "Load page " << page << " from file area" << std::endl;
    } else if (swapArea.count(page) != 0U) {
        std::cout << "Load page " << page << " from swap area" << std::endl;
        swapArea.erase(page);
    } else {
        std::cout << "Allocate new anonymous page " << page << std::endl;
    }

    lruList.push_front(page);
}

/* ================= Unix 风格缺页处理 ================= */

void MemoryManager::accessPage(int page, bool write) {
    if (pageTable.count(page) == 0U) {
        pageTable.emplace(
            page,
            PageTableEntry{
                -1,
                false,
                false,
                fileArea.count(page) != 0U,
                false
            }
        );
    }

    PageTableEntry& pte = pageTable.at(page);

    if (pte.present) {
        std::cout << "Access page " << page << " (hit)" << std::endl;
        lruList.remove(page);
        lruList.push_front(page);
    } else {
        std::cout << "Page fault on page " << page << std::endl;
        swapIn(page);
    }

    if (write) {
        pte.dirty = true;
        std::cout << "Page " << page << " marked dirty" << std::endl;
    }
}