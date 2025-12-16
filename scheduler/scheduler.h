#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <iostream>
#include <queue>
#include <vector>
#include <string>
#include <algorithm>

enum ProcessState { READY, RUNNING, BLOCKED, FINISHED, SUSPENDED };

struct TCB {
    int tid;            // 线程ID (0, 1, 2...)
    std::string state;  // 状态 "RUNNING", "READY" (简化处理)
};

struct PCB {
    std::string pid;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    ProcessState state;
    int startTime = -1;
    int finishTime = -1;
    int priorityLevel;
    int memorySize;
    std::vector<TCB> threads;  // 线程列表
    int currentThreadIdx;      // 当前轮到哪个线程跑
    

    // 银行家算法相关数据结构 (假设系统有3类资源)
    std::vector<int> maxResources;      // Max: 进程最大需求
    std::vector<int> allocatedResources;// Allocation: 当前已分配
    std::vector<int> neededResources;   // Need: 还需要多少 (Max - Allocation)

    PCB(std::string id, int arrival, int burst, int memSize = 0)
        : pid(id), arrivalTime(arrival), burstTime(burst), remainingTime(burst), 
          state(READY), priorityLevel(0), memorySize(memSize == 0 ? burst : memSize),
          currentThreadIdx(0) {
        
        // 默认创建一个主线程 (TID 0)
        threads.push_back({0, "READY"});

        maxResources = {0, 0, 0};
        allocatedResources = {0, 0, 0};
        neededResources = {0, 0, 0};
    }
};

class Scheduler {
public:
    Scheduler();
    
    void createProcess(const std::string& pid, int arrivalTime, int burstTime, int memSize = 0);
    void tick(); 
    int getCurrentTime() const { return globalTime; }
    bool isAllFinished() const;
    PCB* getRunningProcess() const { return runningProcess; }
    void blockCurrentProcess();
    void wakeProcess(PCB* proc);
    
    // 获取所有进程（用于PS命令）
    const std::vector<PCB*>& getAllProcesses() const { return allProcesses; }

    // 初始化系统资源 (A, B, C)
    void setSystemResources(int r1, int r2, int r3);

    // 设置进程的最大资源声明 (Claim)
    bool setProcessMaxRes(PCB* proc, int r1, int r2, int r3);

    // 尝试申请资源 (银行家算法核心)
    // 返回 true 表示安全并分配，false 表示不安全/不足并拒绝
    bool tryRequestResources(PCB* proc, int r1, int r2, int r3);

    // 释放资源
    void releaseResources(PCB* proc, int r1, int r2, int r3);

    // 根据 PID 查找 PCB 指针
    PCB* getProcess(const std::string& pid);

    // 挂起进程（只负责改状态）
    void suspendProcess(const std::string& pid);

    // 激活进程（只负责改状态和入队）
    void activateProcess(const std::string& pid);

    // 给指定进程创建线程
    void createThread(const std::string& pid);

private:
    std::vector<PCB*> allProcesses;     
    std::vector<std::queue<PCB*>> multiLevelQueues;      
    PCB* runningProcess;                
    int globalTime;                     
    const std::vector<int> TIME_SLICES = {1, 2, 4};                      
    int currentSliceUsed;               

    // 【新增】系统当前可用的资源向量 (Available)
    std::vector<int> availableResources;

    // 【新增】安全性检查算法
    bool checkSafety(const std::vector<int>& work, const std::vector<PCB*>& activeProcs);
              
};

#endif // SCHEDULER_H