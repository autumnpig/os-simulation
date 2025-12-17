#include "scheduler.h"
#include <iomanip>

// 构造函数
Scheduler::Scheduler() : runningProcess(nullptr), globalTime(0), currentSliceUsed(0) {
    multiLevelQueues.resize(3); 
    availableResources = {0, 0, 0}; 
    currentAlgorithm = ALG_MLFQ; // 默认为 MLFQ
}

// 设置算法
void Scheduler::setAlgorithm(SchedAlgorithm algo) {
    currentAlgorithm = algo;
}

// 重置调度器
void Scheduler::reset() {
    // 释放旧PCB内存
    // 注意：实际项目中要小心内存管理，这里假设 main 里的逻辑是独立的
    // 如果是 createProcess new 出来的，这里可以不 delete，依赖外部或析构
    // 但为了纯净测试，我们清空容器
    allProcesses.clear(); 
    for(auto& q : multiLevelQueues) {
        while(!q.empty()) q.pop();
    }
    runningProcess = nullptr;
    globalTime = 0;
    currentSliceUsed = 0;
    availableResources = {0, 0, 0};
}

// 创建进程
void Scheduler::createProcess(const std::string& pid, int arrivalTime, int burstTime, int memSize) {
    PCB* newProcess = new PCB(pid, arrivalTime, burstTime, memSize);
    newProcess->priorityLevel = 0; 
    allProcesses.push_back(newProcess);
    // 注意：如果只是做对比测试，可以注释掉这行 log 以保持输出整洁
    // std::cout << "[System] Process " << pid << " created.\n";
}

// 创建线程
void Scheduler::createThread(const std::string& pid) {
    PCB* p = getProcess(pid);
    if (!p) { std::cout << "[Error] Process " << pid << " not found.\n"; return; }
    if (p->state == FINISHED) { std::cout << "[Error] Process finished.\n"; return; }
    int newTid = p->threads.size();
    p->threads.push_back({newTid, "READY"});
    std::cout << "[Thread] Created Thread T" << newTid << " for Process " << pid << ".\n";
}

// 查找进程
PCB* Scheduler::getProcess(const std::string& pid) {
    for (auto p : allProcesses) {
        if (p->pid == pid) return p;
    }
    return nullptr;
}

// 挂起进程
void Scheduler::suspendProcess(const std::string& pid) {
    PCB* p = getProcess(pid);
    if (!p) { std::cout << "[Error] Not found.\n"; return; }
    if (p->state == FINISHED || p->state == SUSPENDED) return;

    if (p == runningProcess) {
        runningProcess = nullptr; 
        currentSliceUsed = 0;
    }
    p->state = SUSPENDED;
    std::cout << "[System] Process " << pid << " SUSPENDED.\n";
}

// 激活进程
void Scheduler::activateProcess(const std::string& pid) {
    PCB* p = getProcess(pid);
    if (!p || p->state != SUSPENDED) return;

    p->state = READY;
    multiLevelQueues[p->priorityLevel].push(p);
    std::cout << "[System] Process " << pid << " ACTIVATED.\n";
}

// --- 核心调度入口 ---
void Scheduler::tick() {
    switch (currentAlgorithm) {
        case ALG_FCFS: tickFCFS(); break;
        case ALG_RR:   tickRR();   break;
        case ALG_MLFQ: tickMLFQ(); break;
    }
    globalTime++;
}

// 1. FCFS 实现
void Scheduler::tickFCFS() {
    // 检查新到达
    for (auto proc : allProcesses) {
        if (proc->arrivalTime == globalTime && proc->state == READY) {
            multiLevelQueues[0].push(proc);
            // std::cout << "[Time " << globalTime << "] " << proc->pid << " arrived.\n";
        }
    }

    // 调度
    if (!runningProcess && !multiLevelQueues[0].empty()) {
        PCB* candidate = multiLevelQueues[0].front();
        if (candidate->state != SUSPENDED) {
            runningProcess = candidate;
            multiLevelQueues[0].pop();
            runningProcess->state = RUNNING;
            if (runningProcess->startTime == -1) runningProcess->startTime = globalTime;
            // std::cout << "[Time " << globalTime << "] " << runningProcess->pid << " RUNNING (FCFS).\n";
        } else {
             multiLevelQueues[0].pop(); // 移除挂起的
        }
    }

    // 执行
    if (runningProcess) {
        runningProcess->remainingTime--;
        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;
            // std::cout << "[Time " << globalTime << "] " << runningProcess->pid << " FINISHED.\n";
            releaseResources(runningProcess, runningProcess->allocatedResources[0], runningProcess->allocatedResources[1], runningProcess->allocatedResources[2]);
            runningProcess = nullptr;
        }
    }
}

// 2. RR 实现 (时间片=2)
void Scheduler::tickRR() {
    int timeSlice = 2; 

    for (auto proc : allProcesses) {
        if (proc->arrivalTime == globalTime && proc->state == READY) {
            multiLevelQueues[0].push(proc);
        }
    }

    if (!runningProcess && !multiLevelQueues[0].empty()) {
        PCB* candidate = multiLevelQueues[0].front();
        multiLevelQueues[0].pop();
        
        if (candidate->state != SUSPENDED) {
            runningProcess = candidate;
            runningProcess->state = RUNNING;
            currentSliceUsed = 0;
            if (runningProcess->startTime == -1) runningProcess->startTime = globalTime;
            // std::cout << "[Time " << globalTime << "] " << runningProcess->pid << " RUNNING (RR).\n";
        }
    }

    if (runningProcess) {
        runningProcess->remainingTime--;
        currentSliceUsed++;

        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;
            releaseResources(runningProcess, runningProcess->allocatedResources[0], runningProcess->allocatedResources[1], runningProcess->allocatedResources[2]);
            runningProcess = nullptr;
        } 
        else if (currentSliceUsed >= timeSlice) {
            runningProcess->state = READY;
            multiLevelQueues[0].push(runningProcess); // 放回队尾
            runningProcess = nullptr;
        }
    }
}

// 3. MLFQ 实现 (原始逻辑)
void Scheduler::tickMLFQ() {
    // 1. 检查新到达
    for (auto proc : allProcesses) {
        if (proc->arrivalTime == globalTime && proc->state == READY) {
            proc->priorityLevel = 0;
            multiLevelQueues[0].push(proc);
            std::cout << "[Time " << globalTime << "] Process " << proc->pid << " arrived -> Queue Level 0.\n";
        }
    }

    // 2. 抢占
    if (runningProcess) {
        for (int i = 0; i < runningProcess->priorityLevel; ++i) {
            if (!multiLevelQueues[i].empty()) {
                std::cout << "[Time " << globalTime << "] Preemption! " << runningProcess->pid 
                          << " yielded.\n";
                runningProcess->state = READY;
                multiLevelQueues[runningProcess->priorityLevel].push(runningProcess);
                runningProcess = nullptr;
                break;
            }
        }
    }

    // 3. 调度
    if (!runningProcess) {
        for (int i = 0; i < 3; ++i) {
            while (!multiLevelQueues[i].empty()) {
                PCB* candidate = multiLevelQueues[i].front();
                if (candidate->state == SUSPENDED) {
                    multiLevelQueues[i].pop(); continue; 
                }
                runningProcess = candidate;
                multiLevelQueues[i].pop();
                runningProcess->state = RUNNING;
                if (runningProcess->startTime == -1) runningProcess->startTime = globalTime;
                currentSliceUsed = 0;
                
                int currentTid = 0;
                if (!runningProcess->threads.empty()) {
                    currentTid = runningProcess->threads[runningProcess->currentThreadIdx].tid;
                }
                std::cout << "[Time " << globalTime << "] Context Switch: " << runningProcess->pid 
                          << " (T" << currentTid << ") RUNNING (L" << i << ").\n";
                break; 
            }
            if (runningProcess) break; 
        }
    }

    // 4. 执行
    if (runningProcess) {
        runningProcess->remainingTime--;
        currentSliceUsed++;
        
        if (!runningProcess->threads.empty()) {
            int tCount = runningProcess->threads.size();
            runningProcess->currentThreadIdx = (runningProcess->currentThreadIdx + 1) % tCount;
        }

        int maxSlice = TIME_SLICES[runningProcess->priorityLevel];
        
        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;
            std::cout << "[Time " << globalTime << "] Process " << runningProcess->pid << " FINISHED.\n";
            releaseResources(runningProcess, runningProcess->allocatedResources[0], runningProcess->allocatedResources[1], runningProcess->allocatedResources[2]);
            runningProcess = nullptr; 
        }
        else if (currentSliceUsed >= maxSlice) {
            int oldLevel = runningProcess->priorityLevel;
            int newLevel = (oldLevel < 2) ? oldLevel + 1 : oldLevel;
            
            runningProcess->priorityLevel = newLevel;
            runningProcess->state = READY;
            multiLevelQueues[newLevel].push(runningProcess);
            
            std::cout << "[Time " << globalTime << "] " << runningProcess->pid 
                      << " slice expired. Demoted L" << oldLevel << " -> L" << newLevel << ".\n";
            runningProcess = nullptr; 
        }
    } else {
        std::cout << "[Time " << globalTime << "] CPU Idle...\n";
    }
}

// 统计计算
SchedStats Scheduler::calculateStats() {
    double totalTurnaround = 0;
    double totalWeighted = 0;
    int count = 0;

    for (auto p : allProcesses) {
        if (p->state == FINISHED) {
            double turn = p->finishTime - p->arrivalTime;
            double weighted = turn / p->burstTime;
            totalTurnaround += turn;
            totalWeighted += weighted;
            count++;
        }
    }
    if (count == 0) return {0.0, 0.0};
    return {totalTurnaround / count, totalWeighted / count};
}

void Scheduler::wakeProcess(PCB* proc) {
    if (proc && proc->state == BLOCKED) {
        proc->state = READY;
        multiLevelQueues[proc->priorityLevel].push(proc);
        std::cout << "[System] " << proc->pid << " Woken up.\n";
    }
}

void Scheduler::blockCurrentProcess() {
    if (runningProcess) {
        runningProcess->state = BLOCKED;
        std::cout << "[System] Process " << runningProcess->pid << " is BLOCKED.\n";
        runningProcess = nullptr; 
        currentSliceUsed = 0;
    }
}

bool Scheduler::isAllFinished() const {
    for (auto proc : allProcesses) {
        if (proc->state != FINISHED) return false;
    }
    return true;
}

// --- 银行家算法  ---
void Scheduler::setSystemResources(int r1, int r2, int r3) {
    availableResources = {r1, r2, r3};
    std::cout << "[Banker] System Resources: " << r1 << " " << r2 << " " << r3 << "\n";
}

bool Scheduler::setProcessMaxRes(PCB* proc, int r1, int r2, int r3) {
    if (!proc) return false;
    proc->maxResources = {r1, r2, r3};
    proc->neededResources = {r1, r2, r3};
    std::cout << "[Banker] " << proc->pid << " Max Claim: " << r1 << " " << r2 << " " << r3 << "\n";
    return true;
}

bool Scheduler::checkSafety(const std::vector<int>& work, const std::vector<PCB*>& activeProcs) {
    std::vector<int> currentWork = work;
    std::vector<bool> finish(activeProcs.size(), false);
    int finishedCount = 0;

    while (finishedCount < activeProcs.size()) {
        bool found = false;
        for (size_t i = 0; i < activeProcs.size(); ++i) {
            if (!finish[i]) {
                bool canProceed = true;
                for (int r = 0; r < 3; ++r) {
                    if (activeProcs[i]->neededResources[r] > currentWork[r]) {
                        canProceed = false; break;
                    }
                }
                if (canProceed) {
                    for (int r = 0; r < 3; ++r) currentWork[r] += activeProcs[i]->allocatedResources[r];
                    finish[i] = true;
                    finishedCount++;
                    found = true;
                }
            }
        }
        if (!found) return false;
    }
    return true;
}

bool Scheduler::tryRequestResources(PCB* proc, int r1, int r2, int r3) {
    if (!proc) return false;
    std::vector<int> request = {r1, r2, r3};
    for (int i = 0; i < 3; ++i) if (request[i] > proc->neededResources[i]) { std::cout << "[Banker] Error: Exceeds Need.\n"; return false; }
    for (int i = 0; i < 3; ++i) if (request[i] > availableResources[i]) { std::cout << "[Banker] Wait: Not available.\n"; return false; }

    std::vector<int> originalAvailable = availableResources;
    std::vector<int> originalAlloc = proc->allocatedResources;
    std::vector<int> originalNeed = proc->neededResources;

    for (int i = 0; i < 3; ++i) {
        availableResources[i] -= request[i];
        proc->allocatedResources[i] += request[i];
        proc->neededResources[i] -= request[i];
    }

    std::vector<PCB*> activeProcs;
    for (auto p : allProcesses) if (p->state != FINISHED) activeProcs.push_back(p);

    if (checkSafety(availableResources, activeProcs)) {
        std::cout << "[Banker] Safe! Allocated.\n";
        return true;
    } else {
        std::cout << "[Banker] Unsafe! Rolled back.\n";
        availableResources = originalAvailable;
        proc->allocatedResources = originalAlloc;
        proc->neededResources = originalNeed;
        return false;
    }
}

void Scheduler::releaseResources(PCB* proc, int r1, int r2, int r3) {
    if (!proc) return;
    std::vector<int> release = {r1, r2, r3};
    bool releasedAny = false;
    for (int i = 0; i < 3; ++i) {
        if (release[i] > 0 && release[i] <= proc->allocatedResources[i]) {
            proc->allocatedResources[i] -= release[i];
            proc->neededResources[i] += release[i];
            availableResources[i] += release[i];
            releasedAny = true;
        }
    }
    // if (releasedAny) std::cout << "[Banker] Released resources.\n";
}