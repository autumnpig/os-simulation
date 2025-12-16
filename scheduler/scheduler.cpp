#include "scheduler.h"
#include <iomanip>

// 构造函数：初始化多级队列，这里初始化为3级
Scheduler::Scheduler() : runningProcess(nullptr), globalTime(0), currentSliceUsed(0) {
    multiLevelQueues.resize(3); 
    // 银行家算法资源默认初始化
    availableResources = {0, 0, 0}; 
}

// 创建进程
void Scheduler::createProcess(const std::string& pid, int arrivalTime, int burstTime, int memSize) {
    PCB* newProcess = new PCB(pid, arrivalTime, burstTime, memSize);
    newProcess->priorityLevel = 0; // 新进程进入最高优先级
    allProcesses.push_back(newProcess);
    std::cout << "[System] Process " << pid << " created (Level 0, Mem: " << newProcess->memorySize << ").\n";
}

// 【新增】创建线程
void Scheduler::createThread(const std::string& pid) {
    PCB* p = getProcess(pid);
    if (!p) {
        std::cout << "[Error] Process " << pid << " not found.\n";
        return;
    }
    if (p->state == FINISHED) {
        std::cout << "[Error] Process finished.\n";
        return;
    }

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
    if (p->state == FINISHED) { std::cout << "[Error] Process finished.\n"; return; }
    if (p->state == SUSPENDED) { std::cout << "[System] Already suspended.\n"; return; }

    // 如果正在运行，强制剥夺
    if (p == runningProcess) {
        runningProcess = nullptr; 
        currentSliceUsed = 0;
        std::cout << "[System] Running process " << pid << " suspended and removed from CPU.\n";
    }

    p->state = SUSPENDED;
    std::cout << "[System] Process " << pid << " SUSPENDED.\n";
}

// 激活进程
void Scheduler::activateProcess(const std::string& pid) {
    PCB* p = getProcess(pid);
    if (!p || p->state != SUSPENDED) {
        std::cout << "[Error] Cannot activate (Not found or not suspended).\n";
        return;
    }

    p->state = READY;
    // 激活后放回原优先级队列
    multiLevelQueues[p->priorityLevel].push(p);
    std::cout << "[System] Process " << pid << " ACTIVATED -> Queue Level " << p->priorityLevel << ".\n";
}

// --- 核心调度逻辑 (MLFQ + Suspend + Threading + Banker) ---
void Scheduler::tick() {
    // 1. 检查新到达的进程
    for (auto proc : allProcesses) {
        if (proc->arrivalTime == globalTime && proc->state == READY) {
            proc->priorityLevel = 0;
            multiLevelQueues[0].push(proc);
            std::cout << "[Time " << globalTime << "] Process " << proc->pid << " arrived -> Queue Level 0.\n";
        }
    }

    // 2. 抢占逻辑 (Preemption)
    if (runningProcess) {
        for (int i = 0; i < runningProcess->priorityLevel; ++i) {
            // 简单的抢占检查：如果高优先级队列非空，就抢占
            // (严谨的话应该检查队列里是否有非挂起的进程，这里简化处理)
            if (!multiLevelQueues[i].empty()) {
                std::cout << "[Time " << globalTime << "] Preemption! " << runningProcess->pid 
                          << " (Level " << runningProcess->priorityLevel << ") yielded.\n";
                runningProcess->state = READY;
                multiLevelQueues[runningProcess->priorityLevel].push(runningProcess);
                runningProcess = nullptr;
                break;
            }
        }
    }

    // 3. 调度逻辑 (从高到低找进程，跳过挂起的)
    if (!runningProcess) {
        for (int i = 0; i < 3; ++i) {
            while (!multiLevelQueues[i].empty()) {
                PCB* candidate = multiLevelQueues[i].front();
                
                // 【挂起检查】如果队首是挂起的，直接移除并跳过
                if (candidate->state == SUSPENDED) {
                    multiLevelQueues[i].pop(); 
                    continue; 
                }
                
                // 找到有效进程
                runningProcess = candidate;
                multiLevelQueues[i].pop();
                
                runningProcess->state = RUNNING;
                if (runningProcess->startTime == -1) runningProcess->startTime = globalTime;
                currentSliceUsed = 0;
                
                // 获取当前线程ID (仅用于显示)
                int currentTid = 0;
                if (!runningProcess->threads.empty()) {
                    currentTid = runningProcess->threads[runningProcess->currentThreadIdx].tid;
                }
                
                std::cout << "[Time " << globalTime << "] Context Switch: " << runningProcess->pid 
                          << " (Thread T" << currentTid << ") RUNNING (Queue Level " << i << ").\n";
                break; // 找到一个就退出 while
            }
            if (runningProcess) break; // 找到一个就退出 for
        }
    }

    // 4. 执行当前进程
    if (runningProcess) {
        runningProcess->remainingTime--;
        currentSliceUsed++;
        
        // 【线程轮转】模拟并发
        if (!runningProcess->threads.empty()) {
            int tCount = runningProcess->threads.size();
            int tIdx = runningProcess->currentThreadIdx;
            // 轮转到下一个线程
            runningProcess->currentThreadIdx = (tIdx + 1) % tCount;
        }

        // 获取当前层级允许的最大时间片
        int maxSlice = TIME_SLICES[runningProcess->priorityLevel];
        
        // 4.1 进程结束
        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;
            std::cout << "[Time " << globalTime << "] Process " << runningProcess->pid << " FINISHED.\n";
            
            // 【银行家算法】自动归还资源
            releaseResources(runningProcess, 
                runningProcess->allocatedResources[0],
                runningProcess->allocatedResources[1],
                runningProcess->allocatedResources[2]);

            runningProcess = nullptr; 
        }
        // 4.2 时间片用完 -> 降级 (MLFQ 核心)
        else if (currentSliceUsed >= maxSlice) {
            int oldLevel = runningProcess->priorityLevel;
            int newLevel = oldLevel;
            
            // 还没到最低级(2)就降级，否则保持
            if (newLevel < 2) newLevel++;
            
            runningProcess->priorityLevel = newLevel;
            runningProcess->state = READY;
            
            // 放回新层级队尾
            multiLevelQueues[newLevel].push(runningProcess);
            
            std::cout << "[Time " << globalTime << "] " << runningProcess->pid 
                      << " slice expired (" << currentSliceUsed << "). Demoted L" 
                      << oldLevel << " -> L" << newLevel << ".\n";
                      
            runningProcess = nullptr; // CPU 空闲
        }
    } else {
        std::cout << "[Time " << globalTime << "] CPU Idle...\n";
    }

    globalTime++;
}

// 唤醒进程
void Scheduler::wakeProcess(PCB* proc) {
    if (proc && proc->state == BLOCKED) {
        proc->state = READY;
        multiLevelQueues[proc->priorityLevel].push(proc);
        std::cout << "[System] " << proc->pid << " Woken up -> Queue Level " << proc->priorityLevel << ".\n";
    }
}

// 阻塞当前进程
void Scheduler::blockCurrentProcess() {
    if (runningProcess) {
        runningProcess->state = BLOCKED;
        std::cout << "[System] Process " << runningProcess->pid << " is BLOCKED.\n";
        runningProcess = nullptr; 
        currentSliceUsed = 0;
    }
}

// 检查是否全部完成
bool Scheduler::isAllFinished() const {
    for (auto proc : allProcesses) {
        if (proc->state != FINISHED) return false;
    }
    return true;
}

// --- 银行家算法实现 (保持不变) ---

void Scheduler::setSystemResources(int r1, int r2, int r3) {
    availableResources = {r1, r2, r3};
    std::cout << "[Banker] System Resources Initialized: A=" << r1 << ", B=" << r2 << ", C=" << r3 << std::endl;
}

bool Scheduler::setProcessMaxRes(PCB* proc, int r1, int r2, int r3) {
    if (!proc) return false;
    proc->maxResources = {r1, r2, r3};
    proc->neededResources = {r1, r2, r3};
    std::cout << "[Banker] Process " << proc->pid << " declared Max Claim: (" << r1 << "," << r2 << "," << r3 << ")\n";
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
                        canProceed = false;
                        break;
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
    for (int i = 0; i < 3; ++i) if (request[i] > proc->neededResources[i]) { std::cout << "[Banker] Error: Request exceeds Need.\n"; return false; }
    for (int i = 0; i < 3; ++i) if (request[i] > availableResources[i]) { std::cout << "[Banker] Wait: Resources not available.\n"; return false; }

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
        std::cout << "[Banker] Safe! Resources allocated to " << proc->pid << ".\n";
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
    if (releasedAny) std::cout << "[Banker] Process " << proc->pid << " released resources.\n";
}