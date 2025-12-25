#include "memory_manager.h"
#include <iostream>
#include <algorithm>

/* ================= 构造函数 ================= */

MemoryManager::MemoryManager(int totalSize, int pageSize, int maxFrames)
    : totalSize(totalSize),
      pageSize(pageSize),
      maxFrames(maxFrames) {
    freeList.push_back({1, totalSize}); // 起始地址设为1，避免 nullptr
    for (int i = 0; i < 10; ++i) fileArea.insert(i);
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

/* ================= 虚拟存储与页面置换 ================= */

void MemoryManager::accessPage(int page, bool write) {
    std::cout << "\n[MMU] Request access page: " << page << (write ? " (Write)" : " (Read)") << "\n";

    if (pageTable.count(page) == 0U) {
        // 首次访问，初始化页表项
        pageTable.emplace(page, PageTableEntry{-1, false, false, fileArea.count(page) != 0U, false});
    }

    PageTableEntry& pte = pageTable.at(page);

    if (pte.present) {
        std::cout << "  -> HIT: Page " << page << " is in Frame " << pte.frame << "\n";
        // LRU 更新：移到队头
        lruList.remove(page);
        lruList.push_front(page);
    } else {
        std::cout << "  -> MISS: Page Fault! Page " << page << " not in memory.\n";
        swapIn(page);
    }

    if (write) {
        pte.dirty = true;
        std::cout << "  -> Mark Page " << page << " as DIRTY.\n";
    }
}

void MemoryManager::swapIn(int page) {
    // 1. 检查物理帧是否已满
    if (lruList.size() >= static_cast<size_t>(maxFrames)) {
        // 淘汰 LRU 链表尾部（最久未使用的）
        int victim = lruList.back();
        lruList.pop_back();
        std::cout << "  -> [Replace] Memory full (" << maxFrames << " frames). Selecting victim: Page " << victim << "\n";
        swapOut(victim);
    }

    // 2. 调入新页
    PageTableEntry& pte = pageTable.at(page);
    pte.frame = static_cast<int>(lruList.size()); // 简单模拟帧号
    pte.present = true;
    pte.inSwap = false;

    // 3. 加入 LRU 队头
    lruList.push_front(page);

    if (pte.fileBacked) {
        std::cout << "  -> [IO] Loaded Page " << page << " from File System.\n";
    } else if (swapArea.count(page)) {
        std::cout << "  -> [IO] Loaded Page " << page << " from Swap Area.\n";
        swapArea.erase(page);
    } else {
        std::cout << "  -> [Alloc] Zero-filled new Page " << page << ".\n";
    }
}

void MemoryManager::swapOut(int page) {
    PageTableEntry& pte = pageTable.at(page);
    pte.present = false;

    if (pte.fileBacked) {
        if (pte.dirty) std::cout << "  -> [IO] Write back dirty Page " << page << " to File.\n";
        else std::cout << "  -> [Drop] Page " << page << " is clean (file-backed), simply drop.\n";
    } else {
        std::cout << "  -> [Swap] Swapped out Page " << page << " to Swap Area.\n";
        swapArea.insert(page);
        pte.inSwap = true;
    }
    pte.dirty = false;
}

// 可视化状态打印
void MemoryManager::printStatus() const {
    std::cout << "\n===== Memory Manager Status =====\n";
    
    // 1. 连续分配状态 (用于 exec/process loading)
    std::cout << "[Partitions (Continuous Alloc)] Total: " << totalSize << "\n";
    std::cout << "  Used Blocks:\n";
    if (usedBlocks.empty()) std::cout << "    (None)\n";
    for (auto& pair : usedBlocks) {
        std::cout << "    | Start: " << std::setw(4) << pair.second.start 
                  << " | Size: " << std::setw(4) << pair.second.size << " |\n";
    }
    
    // 2. 分页状态 (用于 access page demo)
    std::cout << "\n[Paging System (LRU)] Frames Used: " << lruList.size() << "/" << maxFrames << "\n";
    std::cout << "  Physical Frames (LRU Order: Most Recent -> Least Recent):\n  ";
    if (lruList.empty()) std::cout << "(Empty)";
    for (int page : lruList) {
        std::cout << "[Page " << page << "]";
        if (pageTable.at(page).dirty) std::cout << "*"; // 脏页标记
        std::cout << " -> ";
    }
    std::cout << "END\n";

    std::cout << "  Swap Area: { ";
    for (int p : swapArea) std::cout << p << " ";
    std::cout << "}\n";
    std::cout << "=================================\n";
}