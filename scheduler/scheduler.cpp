#include "scheduler.h"
#include <iostream>
#include <algorithm>

// 移除不再需要的 MLFQ 时间片数组
// static const int TIME_SLICES[3] = {2, 4, 8}; 

Scheduler::Scheduler() {
    // 【关键修复】必须初始化这些变量，否则会崩溃
    globalTime = 0;
    nextArrivalIdx = 0;
    runningProcess = nullptr;
    currentAlgorithm = ALG_FCFS;
    
    // 移除 multiLevelQueues.resize(3); 
    // 使用 std::deque<PCB*> readyQueue 不需要预先 resize
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
    // 请确保 PCB 构造函数支持 memSize，或者在这里单独赋值
    PCB* p = new PCB(pid, arrival, burst); 
    p->memSize = memSize; // 如果构造函数没加 memSize，需手动赋值
    p->state = NEW;       

    allProcesses.push_back(p);

    // 按到达时间排序，方便 checkArrivals 处理
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
    if (!p || p->state == FINISHED) return;
    // 确保 PCB 结构体中有 threads 成员
    p->threads.push_back({(int)p->threads.size(), "READY"});
    std::cout << "[System] Thread created for process " << pid << "\n";
}

/* ================= 调度核心 ================= */

void Scheduler::checkArrivals() {
    // 遍历所有尚未到达的进程
    while (nextArrivalIdx < allProcesses.size() &&
           allProcesses[nextArrivalIdx]->arrivalTime <= globalTime) { // 改为 <= 更稳健

        PCB* p = allProcesses[nextArrivalIdx++];
        if (p->state == NEW) {
            p->state = READY;
            // 【修改】使用 readyQueue 而不是 multiLevelQueues[0]
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

            std::cout << "[Time " << globalTime + 1 << "] "
                      << runningProcess->pid << " finished\n";

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
    // 注意：RR模式下阻塞不需要重置 currentSliceUsed，因为下次回来是重新调度的
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
    readyQueue.push_back(p); // 【修改】放入 readyQueue
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

bool Scheduler::tryRequestResources(PCB* p, int r1, int r2, int r3) {
    if (!p) return false;

    std::vector<int> req = {r1, r2, r3};
    for (int i = 0; i < 3; i++) {
        // 检查 1: 请求 > 需求
        if (req[i] > p->neededResources[i]) {
            std::cout << "[Banker] Error: Request exceeds needs.\n";
            return false;
        }
        // 检查 2: 请求 > 可用
        if (req[i] > availableResources[i]) {
            std::cout << "[Banker] Wait: Not enough available resources.\n";
            return false;
        }
    }

    // 假定分配
    for (int i = 0; i < 3; i++) {
        availableResources[i] -= req[i];
        p->allocatedResources[i] += req[i];
        p->neededResources[i] -= req[i];
    }
    
    std::cout << "[Banker] Resources allocated to " << p->pid << "\n";
    return true; 
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