#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <iostream>
#include <queue>
#include <vector>
#include <string>
#include <algorithm>

// 增加进程状态枚举
enum ProcessState { READY, RUNNING, BLOCKED, FINISHED };

struct PCB {
    std::string pid;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    ProcessState state; // 新增状态
    int startTime = -1;
    int finishTime = -1;

    PCB(std::string id, int arrival, int burst)
        : pid(id), arrivalTime(arrival), burstTime(burst), remainingTime(burst), state(READY) {}
};

class Scheduler {
public:
    Scheduler();
    
    // 创建进程并加入全局列表
    void createProcess(const std::string& pid, int arrivalTime, int burstTime);

    // 核心：模拟一个时间单位的流逝
    void tick(); 

    // 获取当前时间
    int getCurrentTime() const { return globalTime; }
    
    // 检查是否所有进程都结束了
    bool isAllFinished() const;

    // 获取当前正在运行的进程（用于信号量记录是谁在等待）
    PCB* getRunningProcess() const { return runningProcess; }

    // 【核心】阻塞当前进程：将 runningProcess 设为 BLOCKED，并从 CPU 移除
    void blockCurrentProcess();

    // 【核心】唤醒指定进程：将进程设为 READY，并放回 readyQueue
    void wakeProcess(PCB* proc);
    
private:
    std::vector<PCB*> allProcesses;     // 所有进程的总账
    std::queue<PCB*> readyQueue;        // 就绪队列
    PCB* runningProcess;                // 当前正在 CPU 跑的进程
    
    int globalTime;                     // 全局时间
    int timeSlice;                      // 时间片（用于RR）
    int currentSliceUsed;               // 当前进程用了多少时间片

    void scheduleRR();                  // 内部调度逻辑
};

#endif // SCHEDULER_H