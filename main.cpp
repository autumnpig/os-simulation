#include <iostream>
#include <string>
#include "scheduler/scheduler.h"
#include "memory_manager/memory_manager.h"
#include "sync/semaphore.h"

// 打印菜单
void printHelp() {
    std::cout << "\n=== OS Simulation Shell ===\n";
    std::cout << "1. step  : Run 1 time unit\n";
    std::cout << "2. run   : Run until all finish\n";
    std::cout << "3. add   : Add a process (e.g. P1 0 5)\n";
    std::cout << "4. mem   : Allocate memory test\n";
    std::cout << "5. lock  : Current process tries to acquire lock (P)\n";
    std::cout << "6. unlock: Current process releases lock (V)\n";
    std::cout << "0. exit  : Exit system\n";
    std::cout << "cmd> ";
}

int main() {
    Scheduler osScheduler;
    Semaphore globalMutex(1); // 创建一个互斥锁（初始值1）
    
    // 预置一些进程
    osScheduler.createProcess("P1", 0, 6);
    osScheduler.createProcess("P2", 2, 3);
    osScheduler.createProcess("P3", 4, 1);

    std::string command;
    while (true) {
        printHelp();
        std::cin >> command;

        if (command == "exit" || command == "0") {
            break;
        } 
        else if (command == "step" || command == "1") {
            osScheduler.tick();
        } 
        else if (command == "run" || command == "2") {
            int idleTicks = 0; // 新增：空闲计数器

            while (!osScheduler.isAllFinished()) {
                osScheduler.tick();
                
                // --- 死锁/无限等待检测 ---
                // 如果当前没有进程在跑 (CPU 空闲)
                if (osScheduler.getRunningProcess() == nullptr) {
                    idleTicks++;
                    // 如果连续空闲超过 10 个时间单位（你可以根据需要调整这个值）
                    if (idleTicks > 10) { 
                        std::cout << "\n[Warning] System Loop Detected! CPU idle for too long.\n";
                        std::cout << "Reason: Possible Deadlock (Blocked processes waiting for locks held by finished processes) or waiting for future processes.\n";
                        std::cout << "Stopping run command.\n";
                        break; // 强制跳出循环
                    }
                } else {
                    idleTicks = 0; // 只要有进程在跑，就重置计数器
                }
                // -----------------------
            }

            if (osScheduler.isAllFinished()) {
                std::cout << "All processes finished!\n";
            }
        }
        else if (command == "add" || command == "3") {
            std::string pid;
            int arr, burst;
            std::cout << "Enter PID ArrivalTime BurstTime: ";
            std::cin >> pid >> arr >> burst;
            osScheduler.createProcess(pid, arr, burst);
        }
        else if (command == "mem" || command == "4") {
            int size;
            std::cout << "Enter size to allocate: ";
            std::cin >> size;
            MemoryManager::allocateMemory(size);
        }
        else if (command == "lock" || command == "5") {
            // 只有当前有进程在跑，才能申请锁
            if (osScheduler.getRunningProcess()) {
                globalMutex.wait(osScheduler);
            } else {
                std::cout << "[Error] No running process to acquire lock.\n";
            }
        }
        else if (command == "unlock" || command == "6") {
            // 模拟当前进程释放锁
            // 注意：真实OS中通常只有持有锁的进程才能释放，这里简化处理
             if (osScheduler.getRunningProcess()) {
                globalMutex.signal(osScheduler);
            } else {
                std::cout << "[Error] No running process to release lock.\n";
            }
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }

    return 0;
}