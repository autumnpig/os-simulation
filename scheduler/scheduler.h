#pragma once
#include <vector>
#include <queue>
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
    ALG_RR,
    ALG_MLFQ
};

struct Thread {
    int tid;
    std::string state;
};

struct PCB {
    std::string pid;
    ProcessState state = NEW;

    int arrivalTime;
    int burstTime;
    int remainingTime;
    int memorySize;

    int startTime  = -1;
    int finishTime = -1;

    int priorityLevel = 0;

    std::vector<Thread> threads;
    int currentThreadIdx = 0;

    std::vector<int> maxResources{0,0,0};
    std::vector<int> allocatedResources{0,0,0};
    std::vector<int> neededResources{0,0,0};

    PCB(const std::string& id, int arr, int burst, int mem)
        : pid(id), arrivalTime(arr), burstTime(burst),
          remainingTime(burst), memorySize(mem) {}
};

struct SchedStats {
    double avgTurnaroundTime;
    double avgWeightedTurnaroundTime;
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

    void suspendProcess(const std::string& pid);
    void activateProcess(const std::string& pid);

    // 算法
    void setAlgorithm(SchedAlgorithm algo);
    void reset();

    // 统计
    SchedStats calculateStats();
    void runAutoComparison();

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
    void tickMLFQ();

    void checkArrivals();             

    bool checkSafety(const std::vector<int>& work,
                     const std::vector<PCB*>& procs);

private:
    std::vector<PCB*> allProcesses;
    std::vector<std::queue<PCB*>> multiLevelQueues;

    PCB* runningProcess = nullptr;

    int globalTime = 0;
    int currentSliceUsed = 0;
    int nextArrivalIdx = 0;            

    std::vector<int> availableResources{0,0,0};
    SchedAlgorithm currentAlgorithm = ALG_MLFQ;
};
