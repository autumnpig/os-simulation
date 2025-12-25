#include "scheduler.h"
#include <iostream>
#include <algorithm>

Scheduler::Scheduler() {
    globalTime = 0;
    nextArrivalIdx = 0;
    runningProcess = nullptr;
    currentAlgorithm = ALG_FCFS;
}

Scheduler::~Scheduler() {
    // 释放所有 PCB 内存
    for (auto p : allProcesses) {
        delete p;
    }
    allProcesses.clear();
}

void Scheduler::setAlgorithm(SchedAlgorithm algo) {
    currentAlgorithm = algo;
}

/* ================= 进程创建 ================= */

// 注意：请确保头文件 scheduler.h 中的 createProcess 声明与这里参数一致
void Scheduler::createProcess(const std::string& pid, int arrival, int burst, int memSize) {
    PCB* p = new PCB(pid, arrival, burst); 
    p->memSize = memSize; 
    p->state = NEW;       

    allProcesses.push_back(p);

    std::sort(allProcesses.begin(), allProcesses.end(),
        [](PCB* a, PCB* b) {
            return a->arrivalTime < b->arrivalTime;
        });

    std::cout << "[System] Process " << pid << " created (NEW)\n";
}

PCB* Scheduler::getProcess(const std::string& pid) {
    for (auto p : allProcesses)
        if (p->pid == pid) return p;
    return nullptr;
}

void Scheduler::createThread(const std::string& pid) {
    PCB* p = getProcess(pid);
    if (!p) {
        std::cout << "[Error] Process " << pid << " not found.\n";
        return;
    }
    if (p->state == FINISHED) {
        std::cout << "[Error] Process " << pid << " is finished.\n";
        return;
    }
    p->threads.push_back({(int)p->threads.size(), "READY"});
    std::cout << "[System] Thread created for process " << pid << "\n";
}

/* ================= 调度核心 ================= */

void Scheduler::checkArrivals() {
    // 遍历所有尚未到达的进程
    while (nextArrivalIdx < allProcesses.size() &&
           allProcesses[nextArrivalIdx]->arrivalTime <= globalTime) {

        PCB* p = allProcesses[nextArrivalIdx++];
        if (p->state == NEW) {
            p->state = READY;
            readyQueue.push_back(p); 
            std::cout << "[Time " << globalTime << "] "
                      << p->pid << " arrived (Ready)\n";
        }
    }
}

void Scheduler::tick() {
    checkArrivals();

    switch (currentAlgorithm) {
        case ALG_FCFS: tickFCFS(); break;
        case ALG_RR:   tickRR();   break;
    }
    globalTime++;
}

/* ================== FCFS ================== */

void Scheduler::tickFCFS() {
    // 1. 尝试调度：如果你没在跑，且队里有人，就选一个
    if (!runningProcess && !readyQueue.empty()) {
        runningProcess = readyQueue.front();
        readyQueue.pop_front();
        runningProcess->state = RUNNING;

        if (runningProcess->startTime == -1)
            runningProcess->startTime = globalTime;

        std::cout << "[Time " << globalTime << "] "
                  << runningProcess->pid << " running\n";
    }

    // 2. 执行进程
    if (runningProcess) {
        runningProcess->remainingTime--;

        // 运行结束
        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;

            releaseResources(runningProcess, 
                     runningProcess->allocatedResources[0],
                     runningProcess->allocatedResources[1],
                     runningProcess->allocatedResources[2]);

            std::cout << "[System] Process " << runningProcess->pid << " finished.\n";
    
            runningProcess = nullptr;
        }
    }
}

/* ================== RR ================== */
void Scheduler::tickRR() {
    const int TIME_SLICE = 2; // 时间片大小

    // 1. 尝试调度
    if (!runningProcess && !readyQueue.empty()) {
        runningProcess = readyQueue.front();
        readyQueue.pop_front();
        runningProcess->state = RUNNING;
        currentSliceUsed = 0; // 重置时间片计数器

        if (runningProcess->startTime == -1)
            runningProcess->startTime = globalTime;

        std::cout << "[Time " << globalTime << "] "
                  << runningProcess->pid << " running\n";
    }

    // 2. 执行进程
    if (runningProcess) {
        runningProcess->remainingTime--;
        currentSliceUsed++;

        // Case A: 进程执行完毕
        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;

            releaseResources(runningProcess, 
                     runningProcess->allocatedResources[0],
                     runningProcess->allocatedResources[1],
                     runningProcess->allocatedResources[2]);

            std::cout << "[Time " << globalTime + 1 << "] "
                      << runningProcess->pid << " finished\n";

            runningProcess = nullptr;
            currentSliceUsed = 0;
        }
        // Case B: 时间片用完，且还没做完 -> 抢占
        else if (currentSliceUsed >= TIME_SLICE) {
            runningProcess->state = READY;
            readyQueue.push_back(runningProcess); // 放回队尾

            std::cout << "[Time " << globalTime + 1 << "] "
                      << runningProcess->pid << " time slice expired -> Ready\n";

            runningProcess = nullptr;
            currentSliceUsed = 0;
        }
    }
}

/* ================= 状态管理 ================= */

void Scheduler::blockCurrentProcess() {
    if (!runningProcess) return;
    runningProcess->state = BLOCKED;
    std::cout << "[System] Process " << runningProcess->pid << " blocked.\n";
    runningProcess = nullptr;
}

void Scheduler::wakeProcess(PCB* proc) {
    if (!proc || proc->state != BLOCKED) return;
    proc->state = READY;
    readyQueue.push_back(proc); // 放入 readyQueue
    std::cout << "[System] Process " << proc->pid << " awakened.\n";
}

void Scheduler::suspendProcess(const std::string& pid) {
    PCB* p = getProcess(pid);
    if (!p || p->state == FINISHED) return;

    if (p == runningProcess) {
        runningProcess = nullptr;
    }
    
    // 如果在就绪队列里，得把它拿出来（这一步比较麻烦，简化起见通常不从容器删除，只改状态）
    // 这里只改状态，调度器取到 SUSPENDED 进程时应该跳过。
    // 但为了简单，我们假设挂起只针对非就绪进程，或者允许挂起状态。
    p->state = SUSPENDED;
    std::cout << "[System] Process " << pid << " suspended.\n";
}

void Scheduler::activateProcess(const std::string& pid) {
    PCB* p = getProcess(pid);
    if (!p || p->state != SUSPENDED) return;

    p->state = READY;
    readyQueue.push_back(p); // 
    std::cout << "[System] Process " << pid << " activated.\n";
}

/* ================= 银行家算法（教学版） ================= */

void Scheduler::setSystemResources(int r1, int r2, int r3) {
    availableResources = {r1, r2, r3};
}

// 确保 PCB 结构体有 maxResources, neededResources, allocatedResources
bool Scheduler::setProcessMaxRes(PCB* p, int r1, int r2, int r3) {
    if (!p) return false;
    p->maxResources = {r1, r2, r3};
    p->neededResources = p->maxResources; 
    // 初始化 allocated 为 0
    p->allocatedResources = {0, 0, 0}; 
    return true;
}

// ================= 银行家算法 (完整版) =================

// 辅助函数：比较两个向量 (v1 <= v2 返回 true)
bool lessOrEqual(const std::vector<int>& v1, const std::vector<int>& v2) {
    for (size_t i = 0; i < v1.size(); ++i) {
        if (v1[i] > v2[i]) return false;
    }
    return true;
}

// 核心算法：安全性检测
bool Scheduler::checkSafety(const std::vector<int>& workAvailable, const std::vector<PCB*>& procs) {
    std::vector<int> work = workAvailable; // 模拟的工作向量
    std::vector<bool> finish(procs.size(), false); // 标记进程是否能完成

    // 1. 预处理：已经结束的进程标记为 true
    for (size_t i = 0; i < procs.size(); ++i) {
        if (procs[i]->state == FINISHED) finish[i] = true;
    }

    // 2. 寻找安全序列
    while (true) {
        bool found = false;
        for (size_t i = 0; i < procs.size(); ++i) {
            // 找到一个：未完成 且 需求 <= 当前可用资源 的进程
            if (!finish[i] && lessOrEqual(procs[i]->neededResources, work)) {
                // 模拟让该进程运行并释放资源
                for (int j = 0; j < 3; ++j) {
                    work[j] += procs[i]->allocatedResources[j];
                }
                finish[i] = true;
                found = true;
                // 找到一个后，从头再找（虽然不是最优，但逻辑最简单）
                break; 
            }
        }
        if (!found) break; // 如果一轮下来没找到任何能执行的进程，退出
    }

    // 3. 检查是否所有进程都能完成
    for (bool f : finish) {
        if (!f) return false; // 只要有一个不能完成，就是不安全状态
    }
    return true;
}

bool Scheduler::tryRequestResources(PCB* p, int r1, int r2, int r3) {
    if (!p) return false;

    std::vector<int> request = {r1, r2, r3};
    
    // 1. 基础检查
    for (int i = 0; i < 3; i++) {
        if (request[i] > p->neededResources[i]) {
            std::cout << "[Banker] Error: Request exceeds declared Max needs.\n";
            return false;
        }
        if (request[i] > availableResources[i]) {
            std::cout << "[Banker] Blocked: Not enough available resources currently.\n";
            return false; // 实际OS中应该让进程阻塞等待，这里简化为请求失败
        }
    }

    // 2. 试探性分配 (Pretend to allocate)
    for (int i = 0; i < 3; i++) {
        availableResources[i] -= request[i];
        p->allocatedResources[i] += request[i];
        p->neededResources[i] -= request[i];
    }

    // 3. 安全性检测 (Banker's Algorithm Core)
    if (checkSafety(availableResources, allProcesses)) {
        std::cout << "[Banker] Safe state! Resources allocated to " << p->pid << ".\n";
        return true;
    } else {
        // 4. 不安全 -> 回滚 (Rollback)
        std::cout << "[Banker] Unsafe state! Allocation denied (Deadlock Avoidance).\n";
        for (int i = 0; i < 3; i++) {
            availableResources[i] += request[i];
            p->allocatedResources[i] -= request[i];
            p->neededResources[i] += request[i];
        }
        return false;
    }
}

void Scheduler::releaseResources(PCB* p, int r1, int r2, int r3) {
    if (!p) return;
    // 简单回收，不检查是否超额释放
    availableResources[0] += r1;
    availableResources[1] += r2;
    availableResources[2] += r3;

    p->allocatedResources[0] -= r1;
    p->allocatedResources[1] -= r2;
    p->allocatedResources[2] -= r3;
    
    // 更新 need? 通常释放资源意味着任务完成或阶段完成，这里暂不反向增加 need
    std::cout << "[Banker] Resources released by " << p->pid << "\n";
}

bool Scheduler::isAllFinished() const {
    if (allProcesses.empty()) return true;
    for (auto p : allProcesses)
        if (p->state != FINISHED)
            return false;
    return true;
}