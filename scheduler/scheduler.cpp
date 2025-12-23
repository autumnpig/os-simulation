#include "scheduler.h"
#include <iostream>
#include <algorithm>
#include <iomanip>

static const int TIME_SLICES[3] = {2, 4, 8};

Scheduler::Scheduler() {
    multiLevelQueues.resize(3);
}

Scheduler::~Scheduler() {
    for (auto p : allProcesses) {
        delete p;
    }
    allProcesses.clear();
}

void Scheduler::reset() {
    for (auto p : allProcesses) {
        delete p;
    }
    allProcesses.clear();

    for (auto& q : multiLevelQueues)
        while (!q.empty()) q.pop();

    runningProcess = nullptr;
    globalTime = 0;
    currentSliceUsed = 0;
    nextArrivalIdx = 0;
    availableResources = {0,0,0};
}

void Scheduler::setAlgorithm(SchedAlgorithm algo) {
    currentAlgorithm = algo;
}

void Scheduler::createProcess(const std::string& pid, int arrival, int burst, int memSize) {
    PCB* p = new PCB(pid, arrival, burst, memSize);
    p->state = NEW;
    allProcesses.push_back(p);

    std::sort(allProcesses.begin(), allProcesses.end(),
        [](PCB* a, PCB* b) {
            return a->arrivalTime < b->arrivalTime;
        });
}

PCB* Scheduler::getProcess(const std::string& pid) {
    for (auto p : allProcesses)
        if (p->pid == pid) return p;
    return nullptr;
}

void Scheduler::createThread(const std::string& pid) {
    PCB* p = getProcess(pid);
    if (!p || p->state == FINISHED) return;
    int tid = p->threads.size();
    p->threads.push_back({tid, "READY"});
}

void Scheduler::checkArrivals() {
    while (nextArrivalIdx < allProcesses.size() &&
           allProcesses[nextArrivalIdx]->arrivalTime == globalTime) {

        PCB* p = allProcesses[nextArrivalIdx];
        p->state = READY;
        p->priorityLevel = 0;
        multiLevelQueues[0].push(p);

        std::cout << "[Time " << globalTime << "] "
                  << p->pid << " arrived.\n";
        nextArrivalIdx++;
    }
}

void Scheduler::tick() {
    checkArrivals();

    switch (currentAlgorithm) {
        case ALG_FCFS: tickFCFS(); break;
        case ALG_RR:   tickRR();   break;
        case ALG_MLFQ: tickMLFQ(); break;
    }
    globalTime++;
}

// ---------- FCFS ----------
void Scheduler::tickFCFS() {
    if (!runningProcess && !multiLevelQueues[0].empty()) {
        runningProcess = multiLevelQueues[0].front();
        multiLevelQueues[0].pop();
        runningProcess->state = RUNNING;
        if (runningProcess->startTime == -1)
            runningProcess->startTime = globalTime;
    }

    if (runningProcess) {
        runningProcess->remainingTime--;
        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;
            releaseResources(runningProcess,
                             runningProcess->allocatedResources[0],
                             runningProcess->allocatedResources[1],
                             runningProcess->allocatedResources[2]);
            runningProcess = nullptr;
        }
    }
}

// ---------- RR ----------
void Scheduler::tickRR() {
    const int slice = 2;

    if (!runningProcess && !multiLevelQueues[0].empty()) {
        runningProcess = multiLevelQueues[0].front();
        multiLevelQueues[0].pop();
        runningProcess->state = RUNNING;
        currentSliceUsed = 0;
        if (runningProcess->startTime == -1)
            runningProcess->startTime = globalTime;
    }

    if (runningProcess) {
        runningProcess->remainingTime--;
        currentSliceUsed++;

        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;
            releaseResources(runningProcess,
                             runningProcess->allocatedResources[0],
                             runningProcess->allocatedResources[1],
                             runningProcess->allocatedResources[2]);
            runningProcess = nullptr;
        }
        else if (currentSliceUsed >= slice) {
            runningProcess->state = READY;
            multiLevelQueues[0].push(runningProcess);
            runningProcess = nullptr;
        }
    }
}

// ---------- MLFQ ----------
void Scheduler::tickMLFQ() {
    if (!runningProcess) {
        for (int i = 0; i < 3; ++i) {
            if (!multiLevelQueues[i].empty()) {
                runningProcess = multiLevelQueues[i].front();
                multiLevelQueues[i].pop();
                runningProcess->state = RUNNING;
                currentSliceUsed = 0;
                if (runningProcess->startTime == -1)
                    runningProcess->startTime = globalTime;
                break;
            }
        }
    }

    if (runningProcess) {
        runningProcess->remainingTime--;
        currentSliceUsed++;

        int maxSlice = TIME_SLICES[runningProcess->priorityLevel];

        if (runningProcess->remainingTime <= 0) {
            runningProcess->state = FINISHED;
            runningProcess->finishTime = globalTime + 1;
            releaseResources(runningProcess,
                             runningProcess->allocatedResources[0],
                             runningProcess->allocatedResources[1],
                             runningProcess->allocatedResources[2]);
            runningProcess = nullptr;
        }
        else if (currentSliceUsed >= maxSlice) {
            int oldLevel = runningProcess->priorityLevel;
            int newLevel = (oldLevel < 2) ? oldLevel + 1 : oldLevel;
            runningProcess->priorityLevel = newLevel;
            runningProcess->state = READY;
            multiLevelQueues[newLevel].push(runningProcess);
            runningProcess = nullptr;
        }
    }
}

// ---------- 统计 ----------
SchedStats Scheduler::calculateStats() {
    double ta = 0, wta = 0;
    int cnt = 0;
    for (auto p : allProcesses) {
        if (p->state == FINISHED) {
            double t = p->finishTime - p->arrivalTime;
            ta += t;
            wta += t / p->burstTime;
            cnt++;
        }
    }
    if (cnt == 0) return {0,0};
    return {ta / cnt, wta / cnt};
}

bool Scheduler::isAllFinished() const {
    for (auto p : allProcesses)
        if (p->state != FINISHED) return false;
    return true;
}
