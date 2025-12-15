#include "scheduler.h"
#include <iomanip>

Scheduler::Scheduler() : runningProcess(nullptr), globalTime(0), timeSlice(2), currentSliceUsed(0) {}

void Scheduler::createProcess(const std::string& pid, int arrivalTime, int burstTime) {
    PCB* newProcess = new PCB(pid, arrivalTime, burstTime);
    allProcesses.push_back(newProcess);
    std::cout << "[System] Process " << pid << " created (Arrival: " << arrivalTime << ", Burst: " << burstTime << ")" << std::endl;
}

// 核心引擎：每调用一次，系统时间+1
void Scheduler::tick() {
    // 1. 检查是否有新进程到达，加入就绪队列
    for (auto proc : allProcesses) {
        if (proc->arrivalTime == globalTime && proc->state == READY) { // 简单处理：状态为READY且刚到时间
            readyQueue.push(proc);
            std::cout << "[Time " << globalTime << "] Process " << proc->pid << " arrived -> Ready Queue." << std::endl;
        }
    }

    // 2. 如果当前没有进程在跑，尝试调度一个
    if (!runningProcess) {
        if (!readyQueue.empty()) {
            runningProcess = readyQueue.front();
            readyQueue.pop();
            runningProcess->state = RUNNING;
            if (runningProcess->startTime == -1) runningProcess->startTime = globalTime;
            currentSliceUsed = 0;
            std::cout << "[Time " << globalTime << "] Context Switch: " << runningProcess->pid << " is now RUNNING." << std::endl;
        }
    }

    // 3. 执行当前进程
    if (runningProcess) {
        runningProcess->remainingTime--;
        currentSliceUsed++;
        std::cout << "[Time " << globalTime << "] " << runningProcess->pid << " running (Remaining: " << runningProcess->remainingTime << ")" << std::endl;

        // 4. 判断进程是否结束
        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;
            std::cout << "[Time " << globalTime << "] Process " << runningProcess->pid << " FINISHED." << std::endl;
            runningProcess = nullptr; // CPU 空闲了
        }
        // 5. 判断时间片是否用完 (RR算法逻辑)
        else if (currentSliceUsed >= timeSlice) {
            std::cout << "[Time " << globalTime << "] Process " << runningProcess->pid << " time slice expired -> Ready Queue." << std::endl;
            runningProcess->state = READY;
            readyQueue.push(runningProcess); // 放回队尾
            runningProcess = nullptr;        // CPU 空闲了，等下一次tick调度新进程
        }
    } else {
        std::cout << "[Time " << globalTime << "] CPU Idle..." << std::endl;
    }

    globalTime++;
}

bool Scheduler::isAllFinished() const {
    for (auto proc : allProcesses) {
        if (proc->state != FINISHED) return false;
    }
    return true;
}