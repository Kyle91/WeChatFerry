#pragma once

#ifndef MEMBER_ADD_MGMT_H
#define MEMBER_ADD_MGMT_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <chrono>

// 成员信息结构
struct MemberEntry {
    std::string member_id;         // 进群人 ID
    std::string member_nickname;   // 进群人昵称
    std::string inviter;           // 邀请人
    std::string system_message;    // 系统消息
    uint32_t  timestamp; // 添加时间戳（秒级）
};

// 系统消息结构
struct SystemMessage {
    std::string group_id;          // 群组 ID
    std::string message_content;   // 消息内容
    uint32_t  timestamp; // 添加时间戳（秒级）
};

// 成员管理类
class MemberAddMgmt {
public:
    // 获取单例实例
    static MemberAddMgmt& GetInstance();

    // 设置通知回调
    void SetNotificationCallback(std::function<void(const std::string& gid, const MemberEntry&)> callback);

    // 添加成员信息并匹配
    void AddMemberEntry(const std::string& gid, MemberEntry& entry);

    // 添加系统消息并匹配
    void AddSystemMessage(const std::string& gid, const std::string& message_content);

    // 启动定时清理任务
    void StartCleanupTask(uint32_t expiration_duration);

private:
    // 私有构造函数
    MemberAddMgmt();

    // 禁止拷贝和赋值
    MemberAddMgmt(const MemberAddMgmt&) = delete;
    MemberAddMgmt& operator=(const MemberAddMgmt&) = delete;

    // 匹配成员消息与系统消息
    void MatchAndNotify(const std::string& gid, const MemberEntry& entry);

    // 匹配系统消息与群成员
    void MatchSystemMessageAndNotify(const SystemMessage& sys_msg);

    // 清理过期数据
    void CleanupExpiredEntries(uint32_t expiration_duration);

    void MatchAllEntries();

    // 数据成员
    std::map<std::string, std::vector<MemberEntry>> group_map; // 群 ID -> 成员列表
    std::vector<SystemMessage> system_messages;               // 系统消息列表
    std::mutex map_mutex;                                     // 互斥锁保护数据
    std::function<void(const std::string& gid, const MemberEntry&)> notification_callback; // 通知回调
    std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> notified_entries; 
    bool ShouldNotify(const std::string& gid, const std::string& member_id, uint32_t current_time, uint32_t threshold);
    std::mutex notified_mutex;
};

#endif // MEMBER_ADD_MGMT_H
