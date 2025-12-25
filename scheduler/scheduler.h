#pragma once

#include <vector>
#include <deque>
#include <string>

// 进程状态枚举
enum ProcessState {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    SUSPENDED,
    FINISHED
};

// 调度算法枚举
enum SchedAlgorithm {
    ALG_FCFS,
    ALG_RR
};

// 线程结构体
struct Thread {
    int tid;
    std::string state;
};

// 进程控制块 (Process Control Block)
struct PCB {
    std::string pid;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int startTime;
    int finishTime;
    ProcessState state;
    
    // 扩展字段
    int memSize; 
    std::vector<Thread> threads;
    
    // 银行家算法资源向量
    std::vector<int> maxResources;
    std::vector<int> allocatedResources;
    std::vector<int> neededResources;

    // 构造函数：初始化所有字段，防止 vector 访问越界
    PCB(std::string id, int arr, int burst) 
        : pid(id), arrivalTime(arr), burstTime(burst), remainingTime(burst), 
          startTime(-1), finishTime(-1), state(NEW), memSize(0),
          maxResources({0, 0, 0}),        // 默认初始化为 0
          allocatedResources({0, 0, 0}),  // 默认初始化为 0
          neededResources({0, 0, 0})      // 默认初始化为 0
    {}
};

class Scheduler {
public:
    Scheduler();
    ~Scheduler();                     

    // 核心调度驱动
    void tick();
    bool isAllFinished() const;

    // --- 进程与线程管理 ---
    void createProcess(const std::string& pid, int arrival, int burst, int memSize = 0);
    void createThread(const std::string& pid);
    
    PCB* getProcess(const std::string& pid);
    PCB* getRunningProcess() const { return runningProcess; }
    const std::vector<PCB*>& getAllProcesses() const { return allProcesses; }
    int getCurrentTime() const { return globalTime; }

    void suspendProcess(const std::string& pid);
    void activateProcess(const std::string& pid);

    // --- 调度算法配置 ---
    void setAlgorithm(SchedAlgorithm algo);

    // --- 银行家算法 (死锁避免) ---
    void setSystemResources(int r1, int r2, int r3);
    bool setProcessMaxRes(PCB* proc, int r1, int r2, int r3);
    bool tryRequestResources(PCB* proc, int r1, int r2, int r3);
    void releaseResources(PCB* proc, int r1, int r2, int r3);

    // --- 同步与阻塞 ---
    void wakeProcess(PCB* proc);
    void blockCurrentProcess();

private:
    // 调度算法具体实现
    void tickFCFS();
    void tickRR();

    // 检查新到达的进程
    void checkArrivals();            

    // 银行家算法安全性检查
    bool checkSafety(const std::vector<int>& work, const std::vector<PCB*>& procs);

private:
    std::vector<PCB*> allProcesses;     // 所有进程列表
    std::deque<PCB*> readyQueue;        // 就绪队列 (使用 deque 支持头部插入等操作)
    PCB* runningProcess = nullptr;      // 当前正在运行的进程

    int globalTime = 0;
    int currentSliceUsed = 0;
    int nextArrivalIdx = 0;            

    std::vector<int> availableResources{0, 0, 0}; // 系统当前可用资源
    SchedAlgorithm currentAlgorithm = ALG_FCFS;   // 默认调度算法
};