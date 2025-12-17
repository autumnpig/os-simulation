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
        case SUSPENDED: return "SUSPENDED"; 
        default: return "UNKNOWN";
    }
}

// 垃圾回收机制
void garbageCollection(Scheduler& scheduler, std::map<std::string, int*>& memMap) {
    const auto& procs = scheduler.getAllProcesses();
    for (auto proc : procs) {
        if (proc->state == FINISHED && memMap.find(proc->pid) != memMap.end()) {
            int* ptr = memMap[proc->pid];
            MemoryManager::freeMemory(ptr); 
            memMap.erase(proc->pid); 
            std::cout << "[GC] Process " << proc->pid << " finished. Memory freed.\n";
        }
    }
}

// --- 算法对比测试函数 ---
void runComparisonTest() {
    std::cout << "\n================ ALGORITHM COMPARISON TEST ================\n";
    std::cout << "Test Workload: \n";
    std::cout << "P1: Arr=0, Burst=5\n";
    std::cout << "P2: Arr=1, Burst=3\n";
    std::cout << "P3: Arr=2, Burst=1\n";
    std::cout << "P4: Arr=3, Burst=7\n";
    std::cout << "-----------------------------------------------------------\n";

    struct AlgoDef { SchedAlgorithm type; std::string name; };
    std::vector<AlgoDef> algos = {
        {ALG_FCFS, "FCFS"},
        {ALG_RR,   "RR (q=2)"},
        {ALG_MLFQ, "MLFQ"}
    };

    std::cout << std::left << std::setw(15) << "Algorithm" 
              << std::setw(20) << "Avg Turnaround" 
              << std::setw(20) << "Avg Weighted" << "\n";
    std::cout << "-----------------------------------------------------------\n";

    for (auto& algo : algos) {
        // 创建独立的测试调度器
        Scheduler tempScheduler;
        tempScheduler.setAlgorithm(algo.type);

        // 添加相同的测试数据
        tempScheduler.createProcess("P1", 0, 5);
        tempScheduler.createProcess("P2", 1, 3);
        tempScheduler.createProcess("P3", 2, 1);
        tempScheduler.createProcess("P4", 3, 7);

        // 静默运行（如果不希望看到详细日志，可以暂时重定向cout，这里为了简单直接跑）
        // 为了避免刷屏，建议在scheduler.cpp里暂时注释掉 createProcess 的 cout
        while (!tempScheduler.isAllFinished()) {
            tempScheduler.tick();
        }

        SchedStats stats = tempScheduler.calculateStats();

        std::cout << "RESULT: " 
                  << std::left << std::setw(15) << algo.name 
                  << std::setw(20) << std::fixed << std::setprecision(2) << stats.avgTurnaroundTime 
                  << std::setw(20) << stats.avgWeightedTurnaroundTime << "\n";
    }
    std::cout << "===========================================================\n";
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
    std::cout << "19. suspend : Suspend Process\n";
    std::cout << "20. activate: Activate Process\n";
    
    std::cout << "--- Threading & Stats ---\n";
    std::cout << "21. thread  : Create Thread\n";
    std::cout << "22. compare : Run Algorithm Comparison Test\n";
    
    std::cout << "cmd> ";
}

int main() {
    Scheduler osScheduler;
    Semaphore globalMutex(1);
    StorageManager disk(1024);
    IPCManager ipc;

    disk.loadFromDisk("os_disk.data");
    osScheduler.setSystemResources(10, 5, 7);
    
    std::map<std::string, int*> processMemoryMap;
    std::string command;

    while (true) {
        garbageCollection(osScheduler, processMemoryMap);
        printHelp();
        std::cin >> command;

        if (command == "exit" || command == "0") {
            disk.saveToDisk("os_disk.data");
            std::cout << "System shutting down...\n";
            break;
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
        else if (command == "add" || command == "3") {
            std::string pid;
            int arr, burst;
            std::cout << "Enter PID ArrivalTime BurstTime: ";
            std::cin >> pid >> arr >> burst;
            osScheduler.createProcess(pid, arr, burst, burst);
        }
        else if (command == "mem" || command == "4") {
            int size;
            std::cout << "Enter size: "; std::cin >> size;
            MemoryManager::allocateMemory(size);
        }
        else if (command == "lock" || command == "5") {
            if (osScheduler.getRunningProcess()) globalMutex.wait(osScheduler);
            else std::cout << "[Error] No running process.\n";
        }
        else if (command == "unlock" || command == "6") {
             if (osScheduler.getRunningProcess()) globalMutex.signal(osScheduler);
            else std::cout << "[Error] No running process.\n";
        }
        else if (command == "touch" || command == "7") {
            std::string name; int size;
            std::cout << "Enter filename size: "; std::cin >> name >> size;
            disk.createFile(name, size);
        }
        else if (command == "rm" || command == "8") {
            std::string name;
            std::cout << "Enter filename: "; std::cin >> name;
            disk.deleteFile(name);
        }
        else if (command == "ls" || command == "9") {
            disk.listFiles();
        }
        else if (command == "exec" || command == "10") {
            std::string filename;
            std::cout << "Enter filename: "; std::cin >> filename;
            int fileSize = disk.getFileSize(filename);
            if (fileSize == -1) {
                std::cout << "[Error] File not found.\n"; continue;
            }
            std::cout << "[Loader] Loading '" << filename << "'...\n";
            int* memPtr = MemoryManager::allocateMemory(fileSize);
            if (!memPtr) {
                std::cout << "[Error] Memory allocation failed.\n"; continue;
            }
            std::string pid = filename;
            if (processMemoryMap.count(pid)) pid += "_copy";
            osScheduler.createProcess(pid, osScheduler.getCurrentTime(), fileSize, fileSize);
            processMemoryMap[pid] = memPtr;
            std::cout << "[Loader] Process '" << pid << "' created.\n";
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
        else if (command == "init_res" || command == "12") {
            int r1, r2, r3;
            std::cout << "Enter (A B C): "; std::cin >> r1 >> r2 >> r3;
            osScheduler.setSystemResources(r1, r2, r3);
        }
        else if (command == "claim" || command == "13") {
            std::string pid; int r1, r2, r3;
            std::cout << "Enter PID Max (A B C): "; std::cin >> pid >> r1 >> r2 >> r3;
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
                std::cout << "Current requests (A B C): "; std::cin >> r1 >> r2 >> r3;
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
        else if (command == "send" || command == "16") {
            PCB* current = osScheduler.getRunningProcess();
            if (current) {
                std::string t, m;
                std::cout << "Target MSG: "; std::cin >> t >> m;
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
        else if (command == "suspend" || command == "19") {
            std::string pid; std::cout << "Enter PID: "; std::cin >> pid;
            PCB* proc = osScheduler.getProcess(pid);
            if (!proc) { std::cout << "[Error] Not found.\n"; continue; }
            if (processMemoryMap.count(pid)) {
                MemoryManager::freeMemory(processMemoryMap[pid]);
                processMemoryMap.erase(pid);
            }
            osScheduler.suspendProcess(pid);
        }
        else if (command == "activate" || command == "20") {
            std::string pid; std::cout << "Enter PID: "; std::cin >> pid;
            PCB* proc = osScheduler.getProcess(pid);
            if (!proc || proc->state != SUSPENDED) { std::cout << "[Error] Cannot activate.\n"; continue; }
            int* newPtr = MemoryManager::allocateMemory(proc->memorySize);
            if (!newPtr) { std::cout << "[Error] Not enough memory!\n"; } 
            else {
                processMemoryMap[pid] = newPtr;
                osScheduler.activateProcess(pid);
            }
        }
        else if (command == "thread" || command == "21") {
            std::string pid; std::cout << "Enter PID: "; std::cin >> pid;
            osScheduler.createThread(pid);
        }
        // --- 对比命令入口 ---
        else if (command == "compare" || command == "22") {
            runComparisonTest();
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }
    return 0;
}