#include "ipc.h"
#include <iomanip>

bool IPCManager::sendMessage(const std::string& fromPid, const std::string& toPid, const std::string& content) {
    Message msg;
    msg.senderPid = fromPid;
    msg.content = content;
    msg.timestamp = 0; // 也可以传入 Scheduler 的全局时间，这里暂存0

    messageQueues[toPid].push(msg);
    std::cout << "[IPC] Message sent from " << fromPid << " to " << toPid << ".\n";
    return true;
}

bool IPCManager::receiveMessage(const std::string& targetPid, Message& outMsg) {
    if (messageQueues.find(targetPid) == messageQueues.end() || messageQueues[targetPid].empty()) {
        return false;
    }

    // 取出队首消息
    outMsg = messageQueues[targetPid].front();
    messageQueues[targetPid].pop();
    
    std::cout << "[IPC] Process " << targetPid << " received message from " << outMsg.senderPid << ".\n";
    return true;
}

bool IPCManager::hasMessage(const std::string& targetPid) const {
    auto it = messageQueues.find(targetPid);
    return (it != messageQueues.end() && !it->second.empty());
}

void IPCManager::printStatus() const {
    std::cout << "\n--- IPC Message Queues ---\n";
    if (messageQueues.empty()) {
        std::cout << "(No messages pending)\n";
    }
    for (const auto& pair : messageQueues) {
        if (!pair.second.empty()) {
            std::cout << "Process " << pair.first << ": " << pair.second.size() << " unread message(s).\n";
        }
    }
    std::cout << "--------------------------\n";
}