#ifndef IPC_H
#define IPC_H

#include <string>
#include <queue>
#include <map>
#include <iostream>
#include <vector>

// 消息结构体
struct Message {
    std::string senderPid;
    std::string content;
    int timestamp;
};

class IPCManager {
public:
    // 发送消息：from -> to
    bool sendMessage(const std::string& fromPid, const std::string& toPid, const std::string& content);

    // 接收消息：获取发给 targetPid 的第一条消息
    // return: 如果有消息返回 true，并填充 outMsg；否则返回 false
    bool receiveMessage(const std::string& targetPid, Message& outMsg);

    // 查看是否有消息待处理
    bool hasMessage(const std::string& targetPid) const;

    // debug: 打印所有消息队列状态
    void printStatus() const;

private:
    // 每个进程都有一个专属的收件箱（队列）
    std::map<std::string, std::queue<Message>> messageQueues;
};

#endif // IPC_H