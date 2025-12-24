#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <map>
#include <limits>
#include <sstream>

// 各模块头文件
#include "scheduler/scheduler.h"
#include "memory_manager/memory_manager.h"
#include "sync/semaphore.h"
#include "storage/storage.h"
#include "ipc/ipc.h"

// 状态转字符串
std::string stateToString(ProcessState s) {
    switch (s) {
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case BLOCKED: return "BLOCKED";
        case FINISHED: return "FINISHED";
        case SUSPENDED: return "SUSPENDED";
        default: return "UNKNOWN";
    }
}

// 垃圾回收
void garbageCollection(Scheduler& scheduler, std::map<std::string, int*>& memMap, MemoryManager& mm) {
    const auto& procs = scheduler.getAllProcesses();
    for (auto p : procs) {
        if (p->state == FINISHED && memMap.count(p->pid)) {
            mm.freeMemory(memMap[p->pid]);
            memMap.erase(p->pid);
            std::cout << "[GC] Process " << p->pid << " memory freed.\n";
        }
    }
}


// 帮助菜单（只打印一次）
void printHelp() {
    std::cout << "\n========== OS Simulation Shell ==========\n";
    std::cout << "[ Basic Control ]\n";
    std::cout << " 1. step      : Run 1 tick\n";
    std::cout << " 2. run       : Run until finish\n";
    std::cout << " 0. exit      : Exit system\n\n";

    std::cout << "[ --------- Process ---------]\n";
    std::cout << " 3. add       : add <pid> <arr> <burst>\n";
    std::cout << " 4. ps        : Show process table\n";
    std::cout << " 5. thread    : thread <pid>\n";
    std::cout << " 6. suspend   : suspend <pid>\n";
    std::cout << " 7. activate  : activate <pid>\n\n";

    std::cout << "[ --------- Memory ---------]\n";
    std::cout << " 8. mem       : mem <size>\n";
    std::cout << " 9. access    : access <page> <0/1>\n\n";

    std::cout << "[ --------- File System ---------]\n";
    std::cout << "10. touch     : touch <name> <size>\n";
    std::cout << "11. rm        : rm <name>\n";
    std::cout << "12. ls        : list files\n";
    std::cout << "13. exec      : exec <filename>\n\n";

    std::cout << "[ --------- IPC ---------]\n";
    std::cout << "20. send      : send <pid> <msg>\n";
    std::cout << "21. recv      : recv\n";
    std::cout << "22. ipcs      : ipc status\n\n";

    std::cout << "Type 'help' to show this menu again.\n";
    std::cout << "=========================================\n";
}

void selectAlgorithmAtRun(Scheduler& scheduler) {
    std::cout << "\nChoose scheduling algorithm:\n";
    std::cout << "1. FCFS\n";
    std::cout << "2. Round Robin\n";
    std::cout << "Input choice: ";

    int choice = 1; // 默认值
    std::string input;
    
    // 读取一行输入
    if (std::getline(std::cin, input) && !input.empty()) {
        try {
            choice = std::stoi(input); 
        } catch (...) {
            choice = 1; 
        }
    }

    switch (choice) {
    case 1:
        scheduler.setAlgorithm(ALG_FCFS);
        std::cout << "Algorithm set to FCFS\n";
        break;
    case 2:
        scheduler.setAlgorithm(ALG_RR);
        std::cout << "Algorithm set to Round Robin\n";
        break;
    default:
        std::cout << "Invalid input, default to FCFS\n";
        scheduler.setAlgorithm(ALG_FCFS);
    }
}


int main() {

    Scheduler osScheduler;
    Semaphore globalMutex(1);
    StorageManager disk(1024);
    IPCManager ipc;
    MemoryManager mm(1024, 32, 16);

//    disk.loadFromDisk("os_disk.data");
    osScheduler.setSystemResources(10, 5, 7);

    std::map<std::string, int*> processMemoryMap;

    // 菜单只显示一次
    printHelp();

    while (true) {
        garbageCollection(osScheduler, processMemoryMap, mm);

        std::cout << "\ncmd> ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n[System] Input stream closed.\n";
            break;
        }

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        // ===== 基础控制 =====
        if (cmd == "exit" || cmd == "0") {
            disk.saveToDisk("os_disk.data");
            std::cout << "System shutting down...\n";
            break;
        }
        else if (cmd == "help" || cmd == "?") {
            printHelp();
        }
        else if (cmd == "step" || cmd == "1") {
            osScheduler.tick();
        }
        else if (cmd == "run" || cmd == "2") {
            selectAlgorithmAtRun(osScheduler);

        std::cout << "\nSystem running...\n";

        while (!osScheduler.isAllFinished()) {
            osScheduler.tick();
        }
        std::cout << "All processes finished.\n";
        }


        // ===== 进程管理 =====
        else if (cmd == "add" || cmd == "3") {
            std::string pid;
            int arr, burst;
            if (!(iss >> pid >> arr >> burst)) {
                std::cout << "[Usage] add <pid> <arr> <burst>\n";
                continue;
            }
            osScheduler.createProcess(pid, arr, burst, burst);
            std::cout << "[System] Process " << pid << " added.\n";
        }
        else if (cmd == "ps" || cmd == "4") {
            const auto& procs = osScheduler.getAllProcesses();
            std::cout << std::left
                      << std::setw(10) << "PID"
                      << std::setw(12) << "State"
                      << std::setw(6) << "Lvl"
                      << std::setw(6) << "Thr"
                      << std::setw(6) << "Rem"
                      << "Mem\n";
            std::cout << "---------------------------------------------\n";
            for (auto p : procs) {
                std::string mem = "N/A";
                if (processMemoryMap.count(p->pid)) mem = "Alloc";
                else if (p->state == FINISHED) mem = "Freed";
                else if (p->state == SUSPENDED) mem = "Swap";

                std::cout << std::left
                          << std::setw(10) << p->pid
                          << std::setw(12) << stateToString(p->state)
                          << std::setw(6) << p->threads.size()
                          << std::setw(6) << p->remainingTime
                          << mem << "\n";
            }
        }
        else if (cmd == "thread" || cmd == "5") {
            std::string pid;
            if (!(iss >> pid)) {
                std::cout << "[Usage] thread <pid>\n";
                continue;
            }
            osScheduler.createThread(pid);
        }

        // ===== 内存 =====
        else if (cmd == "mem" || cmd == "8") {
            int size;
            if (!(iss >> size)) {
                std::cout << "[Usage] mem <size>\n";
                continue;
            }
            mm.allocateMemory(size);
        }
        else if (cmd == "access" || cmd == "9") {
            int page, w;
            if (!(iss >> page >> w)) {
                std::cout << "[Usage] access <page> <0/1>\n";
                continue;
            }
            mm.accessPage(page, w == 1);
        }

        // ===== 文件系统 =====
        else if (cmd == "touch" || cmd == "10") {
            std::string name;
            int size;
            if (!(iss >> name >> size)) {
                std::cout << "[Usage] touch <name> <size>\n";
                continue;
            }
            disk.createFile(name, size);
        }
        else if (cmd == "rm" || cmd == "11") {
            std::string name;
            if (!(iss >> name)) {
                std::cout << "[Usage] rm <name>\n";
                continue;
            }
            disk.deleteFile(name);
        }
        else if (cmd == "ls" || cmd == "12") {
            disk.listFiles();
        }
        else if (cmd == "exec" || cmd == "13") {
            std::string file;
            if (!(iss >> file)) {
                std::cout << "[Usage] exec <filename>\n";
                continue;
            }

            int size = disk.getFileSize(file);
            if (size < 0) {
                std::cout << "[Error] File not found.\n";
                continue;
            }

            int* ptr = mm.allocateMemory(size);
            if (!ptr) {
                std::cout << "[Error] Not enough memory.\n";
                continue;
            }

            // 用时间戳生成唯一 PID
            std::string pid = file + "_" + std::to_string(osScheduler.getCurrentTime());
            osScheduler.createProcess(pid, osScheduler.getCurrentTime(), size, size);
            processMemoryMap[pid] = ptr;

            std::cout << "[Loader] Process " << pid << " created.\n";
        }


        // ===== IPC =====
        else if (cmd == "send" || cmd == "20") {
            PCB* cur = osScheduler.getRunningProcess();
            std::string target, msg;
            if (!cur || !(iss >> target >> msg)) {
                std::cout << "[Usage] send <pid> <msg>\n";
                continue;
            }
            ipc.sendMessage(cur->pid, target, msg);
        }
        else if (cmd == "recv" || cmd == "21") {
            PCB* cur = osScheduler.getRunningProcess();
            if (!cur) {
                std::cout << "[Error] No running process.\n";
                continue;
            }
            Message m;
            if (ipc.receiveMessage(cur->pid, m))
                std::cout << "MSG from " << m.senderPid << ": " << m.content << "\n";
            else
                std::cout << "[IPC] No message.\n";
        }
        else if (cmd == "ipcs" || cmd == "22") {
            ipc.printStatus();
        }

        else {
            std::cout << "Unknown command. Type 'help'.\n";
        }
    }

    return 0;
}
