#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <map>

// 引入各模块头文件
#include "scheduler/scheduler.h"
#include "memory_manager/memory_manager.h"
#include "sync/semaphore.h"
#include "storage/storage.h"
#include "ipc/ipc.h"

// 辅助函数：将状态枚举转为字符串
std::string stateToString(ProcessState s) {
    switch(s) {
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case BLOCKED: return "BLOCKED";
        case FINISHED: return "FINISHED";
        case SUSPENDED: return "SUSPENDED"; // 支持挂起状态
        default: return "UNKNOWN";
    }
}

// 垃圾回收机制：扫描已结束的进程并释放内存
void garbageCollection(Scheduler& scheduler, std::map<std::string, int*>& memMap) {
    const auto& procs = scheduler.getAllProcesses();
    for (auto proc : procs) {
        // 如果进程已结束，且它的内存还在记录表中
        if (proc->state == FINISHED && memMap.find(proc->pid) != memMap.end()) {
            int* ptr = memMap[proc->pid];
            // 归还物理内存
            MemoryManager::freeMemory(ptr); 
            memMap.erase(proc->pid); // 从账本中删除
            std::cout << "[GC] Process " << proc->pid << " finished. Memory freed.\n";
        }
    }
}

void printHelp() {
    std::cout << "\n=== OS Simulation Shell ===\n";
    std::cout << "--- Basic Control ---\n";
    std::cout << "1. step     : Run 1 time unit\n";
    std::cout << "2. run      : Run until all finish\n";
    std::cout << "3. add      : Add process manually (pid arr burst)\n";
    std::cout << "0. exit     : Exit system\n";
    
    std::cout << "--- Resources & Sync ---\n";
    std::cout << "4. mem      : Allocate memory test\n";
    std::cout << "5. lock     : Acquire lock (P)\n";
    std::cout << "6. unlock   : Release lock (V)\n";
    
    std::cout << "--- Storage (Flat) ---\n";
    std::cout << "7. touch    : Create file (name size)\n";
    std::cout << "8. rm       : Delete file (name)\n";
    std::cout << "9. ls       : List files\n";
    std::cout << "10. exec    : Load file & run\n";
    
    std::cout << "--- Process Info ---\n";
    std::cout << "11. ps      : List processes\n";
    
    std::cout << "--- Banker's Algorithm ---\n";
    std::cout << "12. init_res: Set System Resources (A B C)\n";
    std::cout << "13. claim   : Set Max Claim (pid A B C)\n";
    std::cout << "14. req     : Request Resources (A B C)\n";
    std::cout << "15. rinfo   : Show Resource Info\n";
    
    std::cout << "--- IPC ---\n";
    std::cout << "16. send    : Send Message (to_pid msg)\n";
    std::cout << "17. recv    : Receive Message\n";
    std::cout << "18. ipcs    : Show IPC Status\n";

    std::cout << "--- Suspend & Activate ---\n";
    std::cout << "19. suspend : Suspend Process (Swap Out)\n";
    std::cout << "20. activate: Activate Process (Swap In)\n";
    
    std::cout << "--- Threading ---\n";
    std::cout << "21. thread  : Create Thread for Process (pid)\n";
    
    std::cout << "cmd> ";
}

int main() {
    // 1. 初始化各模块
    Scheduler osScheduler;
    Semaphore globalMutex(1);
    StorageManager disk(1024);
    IPCManager ipc;

    // 加载磁盘数据
    disk.loadFromDisk("os_disk.data");

    // 初始化银行家算法默认资源 (示例: 10 5 7)
    osScheduler.setSystemResources(10, 5, 7);
    
    // 进程内存映射表 <PID, 内存地址>
    std::map<std::string, int*> processMemoryMap;

    std::string command;
    while (true) {
        // 每次循环尝试回收已结束进程的内存
        garbageCollection(osScheduler, processMemoryMap);

        printHelp();
        std::cin >> command;

        if (command == "exit" || command == "0") {
            disk.saveToDisk("os_disk.data");
            std::cout << "System shutting down...\n";
            break;
        } 
        
        // === 基础调度 ===
        else if (command == "step" || command == "1") {
            osScheduler.tick();
        } 
        else if (command == "run" || command == "2") {
            while (!osScheduler.isAllFinished()) {
                osScheduler.tick();
            }
            std::cout << "All processes finished!\n";
        }
        else if (command == "add" || command == "3") {
            std::string pid;
            int arr, burst;
            std::cout << "Enter PID ArrivalTime BurstTime: ";
            std::cin >> pid >> arr >> burst;
            // 手动添加的进程，默认内存大小 = BurstTime (简化)
            osScheduler.createProcess(pid, arr, burst, burst);
        }

        // === 内存测试 ===
        else if (command == "mem" || command == "4") {
            int size;
            std::cout << "Enter size to allocate: ";
            std::cin >> size;
            MemoryManager::allocateMemory(size);
        }

        // === 互斥锁 ===
        else if (command == "lock" || command == "5") {
            if (osScheduler.getRunningProcess()) {
                globalMutex.wait(osScheduler);
            } else {
                std::cout << "[Error] No running process.\n";
            }
        }
        else if (command == "unlock" || command == "6") {
             if (osScheduler.getRunningProcess()) {
                globalMutex.signal(osScheduler);
            } else {
                std::cout << "[Error] No running process.\n";
            }
        }

        // === 存储管理 (扁平化) ===
        else if (command == "touch" || command == "7") {
            std::string name;
            int size;
            std::cout << "Enter filename and size: ";
            std::cin >> name >> size;
            disk.createFile(name, size);
        }
        else if (command == "rm" || command == "8") {
            std::string name;
            std::cout << "Enter filename to delete: ";
            std::cin >> name;
            disk.deleteFile(name);
        }
        else if (command == "ls" || command == "9") {
            disk.listFiles();
        }
        
        // === 加载运行 ===
        else if (command == "exec" || command == "10") {
            std::string filename;
            std::cout << "Enter filename to execute: ";
            std::cin >> filename;

            int fileSize = disk.getFileSize(filename);
            if (fileSize == -1) {
                std::cout << "[Error] File not found.\n";
                continue;
            }

            std::cout << "[Loader] Loading '" << filename << "' (Size: " << fileSize << ")...\n";
            int* memPtr = MemoryManager::allocateMemory(fileSize);

            if (memPtr == nullptr) {
                std::cout << "[Error] Memory allocation failed. Try 'suspend' to free space.\n";
                continue;
            }

            std::string pid = filename;
            // 简单的去重
            if (processMemoryMap.count(pid)) pid += "_copy";

            // 创建进程：传入 fileSize 作为所需的内存大小
            osScheduler.createProcess(pid, osScheduler.getCurrentTime(), fileSize, fileSize);
            processMemoryMap[pid] = memPtr;
            
            std::cout << "[Loader] Process '" << pid << "' created.\n";
        }

        // === 进程查看 (PS) ===
        else if (command == "ps" || command == "11") {
            const auto& procs = osScheduler.getAllProcesses();
            std::cout << "\nPID\tState\t\tLvl\tThr\tRem\tMemInfo\n";
            std::cout << "--------------------------------------------------------\n";
            for (auto p : procs) {
                std::string memInfo = "N/A";
                if (processMemoryMap.count(p->pid)) memInfo = "Allocated";
                else if (p->state == FINISHED) memInfo = "Freed";
                else if (p->state == SUSPENDED) memInfo = "Swapped Out"; // 显示换出状态

                std::cout << p->pid << "\t" 
                          << std::left << std::setw(10) << stateToString(p->state) 
                          << "\t" << p->priorityLevel 
                          << "\t" << p->threads.size()
                          << "\t" << p->remainingTime 
                          << "\t" << memInfo << "\n";       
            }
            std::cout << "--------------------------------------------------------\n";
        }

        // === 银行家算法 ===
        else if (command == "init_res" || command == "12") {
            int r1, r2, r3;
            std::cout << "Enter available resources (A B C): ";
            std::cin >> r1 >> r2 >> r3;
            osScheduler.setSystemResources(r1, r2, r3);
        }
        else if (command == "claim" || command == "13") {
            std::string pid;
            int r1, r2, r3;
            std::cout << "Enter PID and Max Claim (A B C): ";
            std::cin >> pid >> r1 >> r2 >> r3;
            
            bool found = false;
            for (auto p : osScheduler.getAllProcesses()) {
                if (p->pid == pid) {
                    osScheduler.setProcessMaxRes(p, r1, r2, r3);
                    found = true; break;
                }
            }
            if (!found) std::cout << "Process not found.\n";
        }
        else if (command == "req" || command == "14") {
            PCB* current = osScheduler.getRunningProcess();
            if (current) {
                int r1, r2, r3;
                std::cout << "Current (" << current->pid << ") requests (A B C): ";
                std::cin >> r1 >> r2 >> r3;
                osScheduler.tryRequestResources(current, r1, r2, r3);
            } else {
                std::cout << "[Error] No running process.\n";
            }
        }
        else if (command == "rinfo" || command == "15") {
             const auto& procs = osScheduler.getAllProcesses();
             std::cout << "\nPID\tMax\tAlloc\tNeed\n";
             for (auto p : procs) {
                 if (p->state != FINISHED) {
                     std::cout << p->pid << "\t"
                               << "(" << p->maxResources[0] << "," << p->maxResources[1] << "," << p->maxResources[2] << ")\t"
                               << "(" << p->allocatedResources[0] << "," << p->allocatedResources[1] << "," << p->allocatedResources[2] << ")\t"
                               << "(" << p->neededResources[0] << "," << p->neededResources[1] << "," << p->neededResources[2] << ")\n";
                 }
             }
        }

        // === IPC ===
        else if (command == "send" || command == "16") {
            PCB* current = osScheduler.getRunningProcess();
            if (current) {
                std::string targetPid, msgContent;
                std::cout << "Target PID: "; std::cin >> targetPid;
                std::cout << "Message: "; std::cin >> msgContent;
                ipc.sendMessage(current->pid, targetPid, msgContent);
            } else {
                std::cout << "[Error] No running process.\n";
            }
        }
        else if (command == "recv" || command == "17") {
            PCB* current = osScheduler.getRunningProcess();
            if (current) {
                Message msg;
                if (ipc.receiveMessage(current->pid, msg)) {
                    std::cout << ">>> MSG from " << msg.senderPid << ": " << msg.content << "\n";
                } else {
                    std::cout << "[IPC] No messages.\n";
                }
            } else {
                std::cout << "[Error] No running process.\n";
            }
        }
        else if (command == "ipcs" || command == "18") {
            ipc.printStatus();
        }

        // === 挂起与激活 (Suspend/Activate) ===
        else if (command == "suspend" || command == "19") {
            std::string pid;
            std::cout << "Enter PID to suspend: ";
            std::cin >> pid;

            PCB* proc = osScheduler.getProcess(pid);
            if (!proc) {
                std::cout << "[Error] Process not found.\n";
                continue;
            }

            // 1. 释放内存 (模拟 Swap Out)
            if (processMemoryMap.count(pid)) {
                int* ptr = processMemoryMap[pid];
                std::cout << "[Swap] Swapping out memory for process " << pid << "...\n";
                MemoryManager::freeMemory(ptr);
                processMemoryMap.erase(pid); // 移除映射
            } else {
                std::cout << "[Warning] Process has no memory allocated (or already swapped out).\n";
            }

            // 2. 更改状态
            osScheduler.suspendProcess(pid);
        }
        else if (command == "activate" || command == "20") {
            std::string pid;
            std::cout << "Enter PID to activate: ";
            std::cin >> pid;

            PCB* proc = osScheduler.getProcess(pid);
            if (!proc || proc->state != SUSPENDED) {
                std::cout << "[Error] Process not found or not suspended.\n";
                continue;
            }

            // 1. 重新申请内存 (模拟 Swap In)
            // 注意：使用 PCB 中记录的 memorySize
            std::cout << "[Swap] Attempting to swap in process " << pid << " (Size: " << proc->memorySize << ")...\n";
            int* newPtr = MemoryManager::allocateMemory(proc->memorySize);

            if (newPtr == nullptr) {
                std::cout << "[Error] Swap In Failed: Not enough memory! Process remains SUSPENDED.\n";
            } else {
                // 2. 记录新地址
                processMemoryMap[pid] = newPtr;
                // 3. 激活状态
                osScheduler.activateProcess(pid);
            }
        }
        else if (command == "thread" || command == "21") {
            std::string pid;
            std::cout << "Enter PID to attach thread: ";
            std::cin >> pid;
            osScheduler.createThread(pid);
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }

    return 0;
}