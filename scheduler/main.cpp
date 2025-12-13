#include "scheduler.h"

int main() {
    Scheduler scheduler;

    // 添加进程
    scheduler.addProcess(PCB("P1", 0, 5));
    scheduler.addProcess(PCB("P2", 2, 3));
    scheduler.addProcess(PCB("P3", 4, 1));
    scheduler.addProcess(PCB("P4", 6, 7));

    // 执行调度算法
    scheduler.FCFS();
    scheduler.RR(2);
    return 0;
}