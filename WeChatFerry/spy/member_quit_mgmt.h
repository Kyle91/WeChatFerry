#pragma once

#ifndef MEMBER_QUIT_MGMT_H
#define MEMBER_QUIT_MGMT_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <chrono>
#include <thread>
#include "receive_msg.h"

// 成员管理类
class MemberQuitMgmt {
public:
    // 获取单例实例
    static MemberQuitMgmt& GetInstance();

    // 设置通知回调
    void SetNotificationCallback(std::function<void(const std::string& gid,uint32_t time, const ModelGroupMember&)> callback);

    // 添加成员退出信息
    void AddMemberQuit(const std::string& gid, std::vector<ModelGroupMember>& list);

    // 析构函数
    ~MemberQuitMgmt();

private:
    // 私有构造函数
    MemberQuitMgmt();

    // 禁止拷贝和赋值
    MemberQuitMgmt(const MemberQuitMgmt&) = delete;
    MemberQuitMgmt& operator=(const MemberQuitMgmt&) = delete;

    // 定时清理旧记录
    void CleanupOldRecords();

    // 数据成员
    std::function<void(const std::string& gid, uint32_t time, const ModelGroupMember&)> notification_callback; // 通知回调
    std::unordered_map<std::string, uint32_t> called_records; // 存储 gid+wxid 和秒级时间戳
    std::mutex record_mutex;   // 保护 called_records 的互斥锁
    std::mutex callback_mutex; // 保护 callback 设置的互斥锁

    // 清理线程
    std::thread cleanup_thread;
    bool stop_cleanup = false; // 用于控制清理线程退出
};

#endif // MEMBER_QUIT_MGMT_H
