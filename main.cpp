#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <map>
#include <limits>
#include <sstream>

// 请确保这些头文件都在对应的文件夹里
#include "scheduler/scheduler.h"
#include "memory_manager/memory_manager.h"
#include "sync/semaphore.h"
#include "storage/storage.h"
#include "ipc/ipc.h"

// 状态转字符串
std::string stateToString(ProcessState s) {
    switch (s) {
        case NEW:       return "NEW";
        case READY:     return "READY";
        case RUNNING:   return "RUNNING";
        case BLOCKED:   return "BLOCKED";
        case FINISHED:  return "FINISHED";
        case SUSPENDED: return "SUSPENDED";
        default:        return "UNKNOWN";
    }
}

// === 核心功能：打印系统当前详细状态 ===
void printSystemStatus(Scheduler& scheduler, const std::map<std::string, int*>& memMap) {
    const auto& procs = scheduler.getAllProcesses();
    std::cout << "\n===== System Status (Time: " << scheduler.getCurrentTime() << ") =====\n";
    
    // 增加了一列 "Thr" (线程数)
    std::cout << std::left 
              << std::setw(8) << "PID" 
              << std::setw(12) << "State" 
              << std::setw(5) << "Thr" 
              << std::setw(8) << "RemTime" 
              << std::setw(10) << "Memory"
              << "Info" << "\n";
    std::cout << "----------------------------------------------------------\n";
    
    if (procs.empty()) {
        std::cout << "(No processes)\n";
    }

    for (auto p : procs) {
        std::string memInfo = "None";
        if (memMap.count(p->pid)) memInfo = "Allocated";
        else if (p->state == FINISHED) memInfo = "Freed";
        else if (p->state == SUSPENDED) memInfo = "Swapped";

        std::cout << std::left 
                  << std::setw(8) << p->pid 
                  << std::setw(12) << stateToString(p->state) 
                  << std::setw(5) << p->threads.size() 
                  << std::setw(8) << p->remainingTime
                  << std::setw(10) << memInfo;
        
        if (p->state == RUNNING) std::cout << "<-- CPU Running";
        if (p->state == BLOCKED) std::cout << "(Waiting)";
        if (p->state == READY)   std::cout << "(In Queue)";
        std::cout << "\n";

        if (!p->threads.empty() && p->state != FINISHED) {
            for (const auto& t : p->threads) {
                std::cout << "  |__ [Thread " << t.tid << "] " 
                          << "State: " << t.state << "\n";
            }
        }
    }
    std::cout << "==========================================================\n";
}

// 垃圾回收：清理已完成进程的内存
void garbageCollection(Scheduler& scheduler, std::map<std::string, int*>& memMap, MemoryManager& mm) {
    const auto& procs = scheduler.getAllProcesses();
    for (auto p : procs) {
        if (p->state == FINISHED && memMap.count(p->pid)) {
            mm.freeMemory(memMap[p->pid]);
            memMap.erase(p->pid);
            // std::cout << "[GC] Process " << p->pid << " memory freed.\n"; // 嫌吵可以注释掉
        }
    }
}

bool checkSystemStalled(Scheduler& scheduler) {
    // 1. 如果所有进程都跑完了，不算僵死，算正常结束
    if (scheduler.isAllFinished()) return false;

    // 2. 检查是否有任何 "能动" 的进程
    const auto& procs = scheduler.getAllProcesses();
    bool hasActiveProcess = false;
    for (auto p : procs) {
        // 只要有一个进程是 NEW, READY 或 RUNNING，系统就是健康的
        if (p->state == NEW || p->state == READY || p->state == RUNNING) {
            hasActiveProcess = true;
            break;
        }
    }

    // 3. 如果没有活跃进程，说明所有未完成的任务都被阻塞了
    return !hasActiveProcess; 
}

void printHelp() {
    std::cout << "\n========== OS Simulation Shell ==========\n";
    
    // 1. 系统运行控制
    std::cout << "[ Execution Control ]\n";
    std::cout << " step (or Enter) : Run 1 tick\n";
    std::cout << " run             : Run until all finished\n";
    std::cout << " switch <1/2>    : Switch Algo (1=FCFS, 2=RR)\n";
    std::cout << " exit            : Exit system\n";

    // 2. 进程管理与状态演示（核心）
    std::cout << "\n[ Process Management ]\n";
    std::cout << " add <pid> <arr> <burst> : Create process manually\n";
    std::cout << " ps              : Show detailed process status\n";
    std::cout << " block           : Block current RUNNING process\n";
    std::cout << " wake <pid>      : Wake up a BLOCKED process\n";
    std::cout << " suspend <pid>   : Suspend process (Swap out)\n";
    std::cout << " active <pid>    : Activate process (Swap in)\n";
    std::cout << " thread <pid>    : Create a thread for process\n";

    // 3. 同步与互斥演示模块
    std::cout << "\n[ Sync & Mutex ]\n";
    std::cout << " lock            : Current process tries to acquire lock (P)\n";
    std::cout << " unlock          : Release lock (V)\n";

    std::cout << "\n[ Banker's Algorithm ]\n";
    std::cout << " res_init <A> <B> <C>: Set system total resources\n";
    std::cout << " res_max <p> <A><B><C>: Set Max need for process\n";
    std::cout << " req <p> <A> <B> <C>  : Request resources\n";

    // 4. 内存管理模块
    std::cout << "\n[ Memory Simulation ]\n";
    std::cout << " mem <size>      : Allocate contiguous memory (Partition)\n";
    std::cout << " access <p> [w]  : Access virtual page <p> (w=write mode)\n";
    std::cout << " mem_stat        : Show detailed memory status\n";

    // 5. 文件系统模块
    std::cout << "\n[ File System ]\n";
    std::cout << " touch <n> <s>   : Create file (name, size)\n";
    std::cout << " ls              : List all files\n";
    std::cout << " rm <name>       : Delete file\n";
    std::cout << " exec <name>     : Load file & create process\n";

    // 6. 进程通信模块
    std::cout << "\n[ IPC (Inter-Process Com) ]\n";
    std::cout << " send <pid> <msg>: Send message to process\n";
    std::cout << " recv            : Receive message (Current Process)\n";
    std::cout << " ipcs            : Show IPC status\n";

    std::cout << "=========================================\n";
}

int main() {
    // 1. 初始化各模块
    Scheduler osScheduler;
    MemoryManager mm(1024, 32, 4);
    StorageManager disk(1024);
    IPCManager ipc;
    Semaphore globalMutex(1); // 演示同步用

    std::map<std::string, int*> processMemoryMap; 

    printHelp();

    while (true) {
        // 自动垃圾回收
        garbageCollection(osScheduler, processMemoryMap, mm);

        std::cout << "\ncmd> ";
        std::string line;
        if (!std::getline(std::cin, line)) break;

        // --- 空行默认是 step (方便连按回车演示) ---
        if (line.empty()) {
            osScheduler.tick();
            printSystemStatus(osScheduler, processMemoryMap);
            continue;
        }

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        // ===== 1. 系统控制 =====
        if (cmd == "exit") {
            break;
        }
        else if (cmd == "help") {
            printHelp();
        }
        else if (cmd == "step") {
            osScheduler.tick();
            printSystemStatus(osScheduler, processMemoryMap);
            if (checkSystemStalled(osScheduler)) {
                std::cout << "\n[Warning] System Stalled! All processes are BLOCKED/SUSPENDED.\n"
                          << "Hint: Use 'wake <pid>' or 'unlock' to resume execution.\n";
            }
        }
        else if (cmd == "run") {
            std::cout << "Running until all finished...\n";
            int max_ticks = 1000; // 防止纯逻辑死循环的保险丝
            int ticks = 0;

            while (!osScheduler.isAllFinished()) {
                // 先检查是否僵死
                if (checkSystemStalled(osScheduler)) {
                     std::cout << "\n[System Stop] Deadlock detected! Stopping auto-run.\n";
                     break; // 强制退出循环，把控制权还给用户
                }

                osScheduler.tick();
                ticks++;

                if (ticks > max_ticks) {
                    std::cout << "\n[System Stop] Time limit exceeded (1000 ticks).\n";
                    break;
                }
            }
            printSystemStatus(osScheduler, processMemoryMap);
        }
        else if (cmd == "switch") {
            int type = 1;
            ss >> type;
            if (type == 2) {
                osScheduler.setAlgorithm(ALG_RR);
                std::cout << "[System] Switched to Round Robin (Slice=2)\n";
            } else {
                osScheduler.setAlgorithm(ALG_FCFS);
                std::cout << "[System] Switched to FCFS\n";
            }
        }

        // ===== 2. 进程状态演示 (答辩核心) =====
        else if (cmd == "add") {
            std::string pid; int arr, burst;
            if (ss >> pid >> arr >> burst) {
                osScheduler.createProcess(pid, arr, burst, 0); // 0 is default mem
            } else {
                std::cout << "Usage: add <pid> <arr> <burst>\n";
            }
        }
        else if (cmd == "ps") {
            printSystemStatus(osScheduler, processMemoryMap);
        }
        else if (cmd == "block") {
            // 手动阻塞当前运行的进程
            osScheduler.blockCurrentProcess();
            printSystemStatus(osScheduler, processMemoryMap); // 立即刷新显示状态
        }
        else if (cmd == "wake") {
            std::string pid;
            if (ss >> pid) {
                osScheduler.wakeProcess(osScheduler.getProcess(pid));
                printSystemStatus(osScheduler, processMemoryMap);
            }
        }
        else if (cmd == "suspend") {
            std::string pid;
            if (ss >> pid) {
                osScheduler.suspendProcess(pid);
                printSystemStatus(osScheduler, processMemoryMap);
            }
        }
        else if (cmd == "active") {
            std::string pid;
            if (ss >> pid) {
                osScheduler.activateProcess(pid);
                printSystemStatus(osScheduler, processMemoryMap);
            }
        }
        else if (cmd == "thread") {
            std::string pid;
            ss >> pid;
            osScheduler.createThread(pid);
        }

        // ===== 同步与互斥演示模块 =====
        else if (cmd == "lock") {
            PCB* current = osScheduler.getRunningProcess();
            if (current) {
                bool success = globalMutex.wait(osScheduler);
                
                printSystemStatus(osScheduler, processMemoryMap);
            } else {
                std::cout << "[Error] No running process to acquire lock.\n";
            }
        }
        else if (cmd == "unlock") {
            globalMutex.signal(osScheduler);
            
            printSystemStatus(osScheduler, processMemoryMap);
        }

        // ===== 银行家算法演示模块 =====
        // 1. 初始化系统资源总量
        else if (cmd == "res_init") {
            int r1, r2, r3;
            if (ss >> r1 >> r2 >> r3) {
                osScheduler.setSystemResources(r1, r2, r3);
                std::cout << "[Banker] System resources set: " << r1 << " " << r2 << " " << r3 << "\n";
            }
        }
        // 2. 设置进程的最大需求 (Max Matrix)
        else if (cmd == "res_max") {
            std::string pid;
            int r1, r2, r3;
            if (ss >> pid >> r1 >> r2 >> r3) {
                PCB* p = osScheduler.getProcess(pid);
                if (osScheduler.setProcessMaxRes(p, r1, r2, r3))
                    std::cout << "[Banker] Max resources set for " << pid << "\n";
                else
                    std::cout << "[Error] Process not found.\n";
            }
        }
        // 3. 进程请求资源 (Request)
        else if (cmd == "req") {
            std::string pid;
            int r1, r2, r3;
            if (ss >> pid >> r1 >> r2 >> r3) {
                PCB* p = osScheduler.getProcess(pid);
                osScheduler.tryRequestResources(p, r1, r2, r3);
            }
        }

        // ===== 3. 内存与文件 (保留原有功能) =====
        else if (cmd == "mem") {
            int size; ss >> size;
            mm.allocateMemory(size);
            // 顺便打印一下状态
            // mm.printStatus(); 
        }
        else if (cmd == "mem_stat") {
            mm.printStatus();
        }
        else if (cmd == "access") {
            // 演示页面置换的核心指令
            int page;
            std::string mode;
            if (ss >> page) {
                bool write = false;
                if (ss >> mode && mode == "w") write = true;
                mm.accessPage(page, write);
            } else {
                std::cout << "Usage: access <page_id> [w]\n";
            }
        }
        
        else if (cmd == "touch") {
            std::string name; int size;
            if (ss >> name >> size) disk.createFile(name, size);
        }
        else if (cmd == "ls") {
            disk.listFiles();
        }
        else if (cmd == "rm") {
            std::string name; ss >> name;
            disk.deleteFile(name);
        }
        else if (cmd == "exec") {
            // 模拟从磁盘加载文件并创建进程
            std::string name;
            if (ss >> name) {
                int size = disk.getFileSize(name);
                if (size > 0) {
                    int* mem = mm.allocateMemory(size);
                    if (mem) {
                        std::string pid = name + "_proc";
                        osScheduler.createProcess(pid, osScheduler.getCurrentTime(), size, size);
                        processMemoryMap[pid] = mem;
                        std::cout << "[Loader] Loaded " << name << " into memory as process " << pid << "\n";
                    } else {
                        std::cout << "[Error] Not enough memory.\n";
                    }
                } else {
                    std::cout << "[Error] File not found.\n";
                }
            }
        }

        // ===== 4. IPC (保留) =====
        else if (cmd == "send") {
            std::string target, msg;
            ss >> target >> msg;
            PCB* cur = osScheduler.getRunningProcess();
            if (cur) ipc.sendMessage(cur->pid, target, msg);
            else std::cout << "[Error] No running process to send message.\n";
        }
        else if (cmd == "recv") {
            PCB* cur = osScheduler.getRunningProcess();
            Message m;
            if (cur && ipc.receiveMessage(cur->pid, m)) {
                std::cout << "[IPC] Recv from " << m.senderPid << ": " << m.content << "\n";
            } else {
                std::cout << "[IPC] No messages.\n";
            }
        }
        else if (cmd == "ipcs") {
            ipc.printStatus();
        }

        else {
            std::cout << "Unknown command. Type 'help'.\n";
        }
    }

    return 0;
}