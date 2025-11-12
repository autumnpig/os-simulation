#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <iostream>
#include <queue>
#include <vector>
#include <string>

/**
 * @brief 进程控制块（Process Control Block）
 */
struct PCB {
    std::string pid;      // 进程 ID
    int arrivalTime;      // 到达时间
    int burstTime;        // 执行时间
    int remainingTime;    // 剩余执行时间

    PCB(std::string id, int arrival, int burst)
        : pid(std::move(id)), arrivalTime(arrival), burstTime(burst), remainingTime(burst) {}
};

/**
 * @brief 调度器类，用于管理和调度进程
 */
class Scheduler {
public:
    /**
     * @brief 向调度队列添加进程
     * @param process 要添加的进程
     */
    void addProcess(const PCB& process);

    /**
     * @brief 执行先来先服务（FCFS）调度算法
     */
    void FCFS();

    /**
     * @brief 执行时间片轮转（RR）调度算法
     * @param timeSlice 时间片大小
     */
    void RR(int timeSlice);

private:
    std::vector<PCB> processList;  // 进程列表
    void printProcessExecution(const std::string& pid, int start, int end); // 输出执行信息
};

#endif // SCHEDULER_H
