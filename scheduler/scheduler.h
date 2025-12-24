#pragma once
#include <vector>
#include <deque>   // 【修改】改为 deque 以支持 FCFS/RR 的队列操作
#include <string>

enum ProcessState {
    NEW,        
    READY,
    RUNNING,
    BLOCKED,
    SUSPENDED,
    FINISHED
};

enum SchedAlgorithm {
    ALG_FCFS,
    ALG_RR
    // ALG_MLFQ // 【删除】不再需要
};

struct Thread {
    int tid;
    std::string state;
};

struct PCB {
    std::string pid;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int startTime;
    int finishTime;
    ProcessState state;
    
    // 兼容 .cpp 所需的字段
    int memSize; 
    std::vector<Thread> threads;
    std::vector<int> maxResources;
    std::vector<int> allocatedResources;
    std::vector<int> neededResources;

    PCB(std::string id, int arr, int burst) 
        : pid(id), arrivalTime(arr), burstTime(burst), remainingTime(burst), 
          startTime(-1), finishTime(-1), state(NEW), memSize(0) {}
};

class Scheduler {
public:
    Scheduler();
    ~Scheduler();                     

    void tick();
    bool isAllFinished() const;

    // 进程 & 线程
    void createProcess(const std::string& pid, int arrival, int burst, int memSize = 0);
    void createThread(const std::string& pid);
    PCB* getProcess(const std::string& pid);
    PCB* getRunningProcess() const { return runningProcess; }
    const std::vector<PCB*>& getAllProcesses() const { return allProcesses; }
    int getCurrentTime() const { return globalTime; }

    void suspendProcess(const std::string& pid);
    void activateProcess(const std::string& pid);

    // 算法
    void setAlgorithm(SchedAlgorithm algo);
    
    // 【删除】reset, calculateStats 等未实现的函数，防止链接错误
    // void reset();
    // SchedStats calculateStats();
    // void runAutoComparison();

    // 银行家
    void setSystemResources(int r1, int r2, int r3);
    bool setProcessMaxRes(PCB* proc, int r1, int r2, int r3);
    bool tryRequestResources(PCB* proc, int r1, int r2, int r3);
    void releaseResources(PCB* proc, int r1, int r2, int r3);

    // 同步 / 阻塞
    void wakeProcess(PCB* proc);
    void blockCurrentProcess();

private:
    // 调度实现
    void tickFCFS();
    void tickRR();
    // void tickMLFQ(); // 【删除】

    void checkArrivals();            

    // bool checkSafety(...); // 【删除】简易版未使用

private:
    std::vector<PCB*> allProcesses;
    
    // 【关键修改】替换为单就绪队列，匹配 .cpp 中的 readyQueue
    std::deque<PCB*> readyQueue; 

    PCB* runningProcess = nullptr;

    int globalTime = 0;
    int currentSliceUsed = 0;
    int nextArrivalIdx = 0;            

    std::vector<int> availableResources{0,0,0};
    SchedAlgorithm currentAlgorithm = ALG_FCFS; // 默认改为 FCFS
};