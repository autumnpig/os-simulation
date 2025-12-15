#include <iostream>
#include <string>
#include "scheduler/scheduler.h"
#include "memory_manager/memory_manager.h"

// 打印菜单
void printHelp() {
    std::cout << "\n=== OS Simulation Shell ===\n";
    std::cout << "1. step  : Run 1 time unit\n";
    std::cout << "2. run   : Run until all finish\n";
    std::cout << "3. add   : Add a process (e.g. P1 0 5)\n";
    std::cout << "4. mem   : Allocate memory test\n";
    std::cout << "0. exit  : Exit system\n";
    std::cout << "cmd> ";
}

int main() {
    Scheduler osScheduler;
    
    // 预置一些进程
    osScheduler.createProcess("P1", 0, 5);
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
            osScheduler.createProcess(pid, arr, burst);
        }
        else if (command == "mem" || command == "4") {
            int size;
            std::cout << "Enter size to allocate: ";
            std::cin >> size;
            MemoryManager::allocateMemory(size);
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }

    return 0;
}