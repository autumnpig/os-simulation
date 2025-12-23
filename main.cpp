// [main.cpp]
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <map>
#include <limits> 

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
        case SUSPENDED: return "SUSPENDED"; 
        default: return "UNKNOWN";
    }
}

// 垃圾回收机制 (使用 MemoryManager 实例)
void garbageCollection(Scheduler& scheduler, std::map<std::string, int*>& memMap, MemoryManager& mm) {
    const auto& procs = scheduler.getAllProcesses();
    for (auto proc : procs) {
        if (proc->state == FINISHED && memMap.find(proc->pid) != memMap.end()) {
            int* ptr = memMap[proc->pid];
            mm.freeMemory(ptr); 
            memMap.erase(proc->pid); 
            std::cout << "[GC] Process " << proc->pid << " finished. Memory freed.\n";
        }
    }
}

// 优化后的帮助菜单
void printHelp() {
    std::cout << "\n========== OS Simulation Shell ==========\n";
    
    std::cout << "[ 基础控制 / Basic Control ]\n";
    std::cout << "  1. step      : 单步执行 (Run 1 tick)\n";
    std::cout << "  2. run       : 运行到底 (Run until finish)\n";
    std::cout << "  0. exit      : 退出系统\n";

    std::cout << "[ 进程与线程 / Process & Thread ]\n";
    std::cout << "  3. add       : 手动添加进程 (pid arr burst)\n";
    std::cout << " 11. ps        : 查看进程状态表\n";
    std::cout << " 21. thread    : 创建用户级线程 (pid)\n";
    std::cout << " 19. suspend   : 挂起进程 (Swap Out)\n";
    std::cout << " 20. activate  : 激活进程 (Swap In)\n";

    std::cout << "[ 内存管理 / Memory Management ]\n";
    std::cout << "  4. mem       : 申请内存测试 (size)\n";
    std::cout << " 23. access    : 模拟页面访问 (page_id is_write[0/1])\n";

    std::cout << "[ 文件系统 / File System ]\n";
    std::cout << "  7. touch     : 创建文件 (name size)\n";
    std::cout << "  8. rm        : 删除文件 (name)\n";
    std::cout << "  9. ls        : 文件列表\n";
    std::cout << " 10. exec      : 加载文件为进程 (filename)\n";

    std::cout << "[ 资源与死锁 / Resources & Deadlock ]\n";
    std::cout << " 12. init_res  : 初始化系统资源 (A B C)\n";
    std::cout << " 13. claim     : 声明最大资源 (pid A B C)\n";
    std::cout << " 14. req       : 申请资源 (A B C)\n";
    std::cout << " 15. rinfo     : 查看资源分配详情\n";
    std::cout << "  5. lock      : 获取全局锁 (P操作)\n";
    std::cout << "  6. unlock    : 释放全局锁 (V操作)\n";

    std::cout << "[ 进程通信 / IPC ]\n";
    std::cout << " 16. send      : 发送消息 (to_pid msg)\n";
    std::cout << " 17. recv      : 接收消息\n";
    std::cout << " 18. ipcs      : 查看消息队列\n";

    std::cout << "[ 性能分析 / Analysis ]\n";
    std::cout << " 22. compare   : 调度算法性能对比测试\n";
    
    std::cout << "=========================================\n";
}

// 智能提示辅助函数
void promptIfEmpty(const std::string& promptText) {
    while (std::isspace(std::cin.peek()) && std::cin.peek() != '\n' && std::cin.peek() != '\r') {
        std::cin.ignore();
    }
    if (std::cin.peek() == '\n' || std::cin.peek() == '\r') {
        std::cout << promptText;
    }
}

int main() {
    Scheduler osScheduler;
    Semaphore globalMutex(1);
    StorageManager disk(1024);
    IPCManager ipc;
    // 实例化新版内存管理器: 总大小1024, 页大小32, 最大物理帧16
    MemoryManager mm(1024, 32, 16);

    disk.loadFromDisk("os_disk.data");
    osScheduler.setSystemResources(10, 5, 7);
    
    std::map<std::string, int*> processMemoryMap;
    std::string command;

    printHelp();

    while (true) {
        garbageCollection(osScheduler, processMemoryMap, mm);
        
        std::cout << "\ncmd> ";
        std::cin >> command;

        // --- 基础控制 ---
        if (command == "exit" || command == "0") {
            disk.saveToDisk("os_disk.data");
            std::cout << "System shutting down...\n";
            break;
        }
        else if (command == "help" || command == "?") {
            printHelp();
        } 
        else if (command == "step" || command == "1") {
            osScheduler.tick();
        } 
        else if (command == "run" || command == "2") {
            while (!osScheduler.isAllFinished()) {
                osScheduler.tick();
            }
            std::cout << "All processes finished!\n";
        }

        // --- 进程与线程 ---
        else if (command == "add" || command == "3") {
            promptIfEmpty("Enter PID ArrivalTime BurstTime: ");
            std::string pid;
            int arr, burst;
            std::cin >> pid >> arr >> burst;
            osScheduler.createProcess(pid, arr, burst, burst);
            std::cout << "[System] Process " << pid << " added manually.\n";
        }
        else if (command == "ps" || command == "11") {
            const auto& procs = osScheduler.getAllProcesses();
            std::cout << "\nPID\tState\t\tLvl\tThr\tRem\tMemInfo\n";
            std::cout << "--------------------------------------------------------\n";
            for (auto p : procs) {
                std::string memInfo = "N/A";
                if (processMemoryMap.count(p->pid)) memInfo = "Allocated";
                else if (p->state == FINISHED) memInfo = "Freed";
                else if (p->state == SUSPENDED) memInfo = "Swapped Out";
                std::cout << p->pid << "\t" 
                          << std::left << std::setw(10) << stateToString(p->state) 
                          << "\t" << p->priorityLevel 
                          << "\t" << p->threads.size()
                          << "\t" << p->remainingTime 
                          << "\t" << memInfo << "\n";       
            }
            std::cout << "--------------------------------------------------------\n";
        }
        else if (command == "thread" || command == "21") {
            promptIfEmpty("Enter PID: ");
            std::string pid; std::cin >> pid;
            osScheduler.createThread(pid);
        }
        else if (command == "suspend" || command == "19") {
            promptIfEmpty("Enter PID: ");
            std::string pid; std::cin >> pid;
            PCB* proc = osScheduler.getProcess(pid);
            if (!proc) { std::cout << "[Error] Not found.\n"; continue; }
            if (processMemoryMap.count(pid)) {
                mm.freeMemory(processMemoryMap[pid]); // 模拟换出内存
                processMemoryMap.erase(pid);
            }
            osScheduler.suspendProcess(pid);
        }
        else if (command == "activate" || command == "20") {
            promptIfEmpty("Enter PID: ");
            std::string pid; std::cin >> pid;
            PCB* proc = osScheduler.getProcess(pid);
            if (!proc || proc->state != SUSPENDED) { std::cout << "[Error] Cannot activate.\n"; continue; }
            
            int* newPtr = mm.allocateMemory(proc->memorySize); // 模拟换入内存
            if (!newPtr) { std::cout << "[Error] Not enough memory!\n"; } 
            else {
                processMemoryMap[pid] = newPtr;
                osScheduler.activateProcess(pid);
            }
        }

        // --- 内存管理 ---
        else if (command == "mem" || command == "4") {
            promptIfEmpty("Enter size: ");
            int size; std::cin >> size;
            mm.allocateMemory(size);
        }
        else if (command == "access" || command == "23") {
            promptIfEmpty("Enter PageID IsWrite(0/1): ");
            int pageId, isWriteInt;
            std::cin >> pageId >> isWriteInt;
            mm.accessPage(pageId, (isWriteInt == 1));
        }

        // --- 文件系统 ---
        else if (command == "touch" || command == "7") {
            promptIfEmpty("Enter filename size: ");
            std::string name; int size;
            std::cin >> name >> size;
            disk.createFile(name, size);
        }
        else if (command == "rm" || command == "8") {
            promptIfEmpty("Enter filename: ");
            std::string name; std::cin >> name;
            disk.deleteFile(name);
        }
        else if (command == "ls" || command == "9") {
            disk.listFiles();
        }
        else if (command == "exec" || command == "10") {
            promptIfEmpty("Enter filename: ");
            std::string filename; std::cin >> filename;
            int fileSize = disk.getFileSize(filename);
            if (fileSize == -1) {
                std::cout << "[Error] File not found.\n"; continue;
            }
            std::cout << "[Loader] Loading '" << filename << "'...\n";
            int* memPtr = mm.allocateMemory(fileSize);
            if (!memPtr) {
                std::cout << "[Error] Memory allocation failed.\n"; continue;
            }
            std::string pid = filename;
            if (processMemoryMap.count(pid)) pid += "_copy";
            osScheduler.createProcess(pid, osScheduler.getCurrentTime(), fileSize, fileSize);
            processMemoryMap[pid] = memPtr;
            std::cout << "[Loader] Process '" << pid << "' created.\n";
        }

        // --- 资源与同步 ---
        else if (command == "init_res" || command == "12") {
            promptIfEmpty("Enter (A B C): ");
            int r1, r2, r3; std::cin >> r1 >> r2 >> r3;
            osScheduler.setSystemResources(r1, r2, r3);
        }
        else if (command == "claim" || command == "13") {
            promptIfEmpty("Enter PID Max (A B C): ");
            std::string pid; int r1, r2, r3;
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
                promptIfEmpty("Current requests (A B C): ");
                int r1, r2, r3; std::cin >> r1 >> r2 >> r3;
                osScheduler.tryRequestResources(current, r1, r2, r3);
            } else std::cout << "[Error] No running process.\n";
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
        else if (command == "lock" || command == "5") {
            if (osScheduler.getRunningProcess()) globalMutex.wait(osScheduler);
            else std::cout << "[Error] No running process.\n";
        }
        else if (command == "unlock" || command == "6") {
             if (osScheduler.getRunningProcess()) globalMutex.signal(osScheduler);
            else std::cout << "[Error] No running process.\n";
        }

        // --- 通信 IPC ---
        else if (command == "send" || command == "16") {
            PCB* current = osScheduler.getRunningProcess();
            if (current) {
                promptIfEmpty("Target MSG: ");
                std::string t, m; std::cin >> t >> m;
                ipc.sendMessage(current->pid, t, m);
            } else std::cout << "[Error] No running process.\n";
        }
        else if (command == "recv" || command == "17") {
            PCB* current = osScheduler.getRunningProcess();
            if (current) {
                Message msg;
                if (ipc.receiveMessage(current->pid, msg)) {
                    std::cout << ">>> MSG from " << msg.senderPid << ": " << msg.content << "\n";
                } else std::cout << "[IPC] No messages.\n";
            } else std::cout << "[Error] No running process.\n";
        }
        else if (command == "ipcs" || command == "18") {
            ipc.printStatus();
        }

        // --- 性能分析 ---
        else if (command == "compare" || command == "22") {
            // 直接调用调度器内部封装好的对比逻辑
            osScheduler.runAutoComparison();
        }
        else {
            std::cout << "Unknown command. Type 'help' for list.\n";
        }
    }
    return 0;
}