#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <iostream>
#include <queue>
#include <vector>
#include <string>
#include <algorithm>

// --- 调度算法枚举 ---
enum SchedAlgorithm { ALG_FCFS, ALG_RR, ALG_MLFQ };

// --- 统计数据结构 ---
struct SchedStats {
    double avgTurnaroundTime;        // 平均周转时间
    double avgWeightedTurnaroundTime;// 平均带权周转时间
};

enum ProcessState { READY, RUNNING, BLOCKED, FINISHED, SUSPENDED };

struct TCB {
    int tid;            // 线程ID
    std::string state;  // 状态
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
    
    // 银行家算法相关
    std::vector<int> maxResources;      
    std::vector<int> allocatedResources;
    std::vector<int> neededResources;   

    PCB(std::string id, int arrival, int burst, int memSize = 0)
        : pid(id), arrivalTime(arrival), burstTime(burst), remainingTime(burst), 
          state(READY), priorityLevel(0), memorySize(memSize == 0 ? burst : memSize),
          currentThreadIdx(0) {
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
    
    const std::vector<PCB*>& getAllProcesses() const { return allProcesses; }

    // 银行家算法
    void setSystemResources(int r1, int r2, int r3);
    bool setProcessMaxRes(PCB* proc, int r1, int r2, int r3);
    bool tryRequestResources(PCB* proc, int r1, int r2, int r3);
    void releaseResources(PCB* proc, int r1, int r2, int r3);

    PCB* getProcess(const std::string& pid);
    void suspendProcess(const std::string& pid);
    void activateProcess(const std::string& pid);
    void createThread(const std::string& pid);

    // --- 对比分析相关接口 ---
    void setAlgorithm(SchedAlgorithm algo); // 设置算法
    SchedStats calculateStats();            // 计算统计结果
    void reset();                           // 重置调度器状态（用于重新测试）

private:
    std::vector<PCB*> allProcesses;     
    std::vector<std::queue<PCB*>> multiLevelQueues;      
    PCB* runningProcess;                
    int globalTime;                     
    const std::vector<int> TIME_SLICES = {1, 2, 4};                      
    int currentSliceUsed;               

    std::vector<int> availableResources;

    bool checkSafety(const std::vector<int>& work, const std::vector<PCB*>& activeProcs);

    // --- 当前算法模式 ---
    SchedAlgorithm currentAlgorithm;

    // --- 具体算法实现 ---
    void tickFCFS();
    void tickRR();
    void tickMLFQ();
};

#endif // SCHEDULER_H