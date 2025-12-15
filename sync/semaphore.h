#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <queue>
#include <iostream>
#include "../scheduler/scheduler.h"

class Semaphore {
private:
    int value; // 信号量的值（1表示互斥锁，N表示资源数）
    std::queue<PCB*> waitQueue; // 等待这个信号量的进程队列

public:
    Semaphore(int initValue = 1) : value(initValue) {}

    // P操作 (Wait / Acquire)
    // 返回值：true 表示继续执行，false 表示被阻塞了
    bool wait(Scheduler& scheduler) {
        value--;
        if (value < 0) {
            // 资源不足，需要阻塞当前进程
            PCB* current = scheduler.getRunningProcess();
            if (current) {
                waitQueue.push(current);      // 1. 加入等待队列
                scheduler.blockCurrentProcess(); // 2. 通知调度器阻塞它
                std::cout << "[Sync] Process " << current->pid << " waits (Sem=" << value << ") -> Blocked." << std::endl;
                return false;
            }
        }
        std::cout << "[Sync] Resource acquired (Sem=" << value << ")." << std::endl;
        return true;
    }

    // V操作 (Signal / Release)
    void signal(Scheduler& scheduler) {
        value++;
        if (value <= 0) {
            // 说明还有人在排队，需要唤醒一个
            if (!waitQueue.empty()) {
                PCB* wakeP = waitQueue.front();
                waitQueue.pop();
                scheduler.wakeProcess(wakeP); // 通知调度器唤醒它
                std::cout << "[Sync] Process " << wakeP->pid << " signaled (Sem=" << value << ")." << std::endl;
            }
        } else {
            std::cout << "[Sync] Resource released (Sem=" << value << ")." << std::endl;
        }
    }
    
    int getValue() const { return value; }
};

#endif // SEMAPHORE_H