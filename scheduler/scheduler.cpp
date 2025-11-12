#include "scheduler.h"
#include <algorithm>
#include <iomanip>

/**
 * @brief 向调度队列添加进程
 */
void Scheduler::addProcess(const PCB& process) {
    processList.push_back(process);
}

/**
 * @brief 打印进程执行信息
 */
void Scheduler::printProcessExecution(const std::string& pid, int start, int end) {
    std::cout << std::setw(8) << pid
              << " | Start: " << std::setw(3) << start
              << " | End: " << std::setw(3) << end << std::endl;
}

/**
 * @brief 先来先服务（FCFS）调度算法实现
 */
void Scheduler::FCFS() {
    if (processList.empty()) {
        std::cout << "No processes to schedule." << std::endl;
        return;
    }

    // 按到达时间排序
    std::sort(processList.begin(), processList.end(),
              [](const PCB& a, const PCB& b) { return a.arrivalTime < b.arrivalTime; });

    std::cout << "\n===== FCFS Scheduling =====" << std::endl;

    int currentTime = 0;
    for (auto& process : processList) {
        if (currentTime < process.arrivalTime)
            currentTime = process.arrivalTime; // 等待进程到达

        int start = currentTime;
        currentTime += process.burstTime;
        int end = currentTime;

        printProcessExecution(process.pid, start, end);
    }
}

/**
 * @brief 时间片轮转（RR）调度算法实现
 */
void Scheduler::RR(int timeSlice) {
    if (processList.empty()) {
        std::cout << "No processes to schedule." << std::endl;
        return;
    }

    // 按到达时间排序
    std::sort(processList.begin(), processList.end(),
              [](const PCB& a, const PCB& b) { return a.arrivalTime < b.arrivalTime; });

    std::queue<PCB> readyQueue;
    int currentTime = 0;
    size_t index = 0; // 下一个待加入就绪队列的进程索引

    std::cout << "\n===== Round Robin Scheduling (Time Slice = " << timeSlice << ") =====" << std::endl;

    while (!readyQueue.empty() || index < processList.size()) {
        // 将到达的进程加入队列
        while (index < processList.size() && processList[index].arrivalTime <= currentTime) {
            readyQueue.push(processList[index]);
            index++;
        }

        if (readyQueue.empty()) {
            currentTime = processList[index].arrivalTime; // 跳到下一个进程的到达时间
            continue;
        }

        PCB current = readyQueue.front();
        readyQueue.pop();

        int start = currentTime;
        int execTime = std::min(timeSlice, current.remainingTime);
        current.remainingTime -= execTime;
        currentTime += execTime;
        int end = currentTime;

        printProcessExecution(current.pid, start, end);

        // 检查在执行过程中是否有新进程到达
        while (index < processList.size() && processList[index].arrivalTime <= currentTime) {
            readyQueue.push(processList[index]);
            index++;
        }

        // 若该进程未完成，放回队列尾部
        if (current.remainingTime > 0) {
            readyQueue.push(current);
        }
    }
}
