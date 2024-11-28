#include "member_quit_mgmt.h"
#include <set>
#include <chrono>
#include <thread>
#include <future>
#include <iostream>
#include "util.h"

// 单例实例
MemberQuitMgmt& MemberQuitMgmt::GetInstance() {
    static MemberQuitMgmt instance;
    return instance;
}

// 私有构造函数
MemberQuitMgmt::MemberQuitMgmt() : notification_callback(nullptr) {
    // 启动清理线程
    cleanup_thread = std::thread(&MemberQuitMgmt::CleanupOldRecords, this);
}

// 析构函数，清理资源
MemberQuitMgmt::~MemberQuitMgmt() {
    stop_cleanup = true;
    if (cleanup_thread.joinable()) {
        cleanup_thread.join();
    }
}

// 设置通知回调函数
void MemberQuitMgmt::SetNotificationCallback(std::function<void(const std::string& gid, uint32_t time, const ModelGroupMember&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    notification_callback = callback;
}

// 添加成员退出信息
void MemberQuitMgmt::AddMemberQuit(const std::string& gid, std::vector<ModelGroupMember>& list) {
    std::lock_guard<std::mutex> lock(record_mutex);

    uint32_t time = GetCurrentTimestamp();

    for (const auto& member : list) {
        // 使用 gid + wxid 作为键
        std::string key = gid + member.Wxid;

        // 检查是否已经回调过
        if (called_records.find(key) == called_records.end()) {
            // 记录当前时间戳
            called_records[key] = time;

            // 执行回调
            if (notification_callback) {
                notification_callback(gid, time, member);
            }
        }
    }
}

// 定时清理超时的记录
void MemberQuitMgmt::CleanupOldRecords() {
    while (!stop_cleanup) {
        std::this_thread::sleep_for(std::chrono::minutes(1)); // 每分钟清理一次

        uint32_t current_time = GetCurrentTimestamp();

        std::lock_guard<std::mutex> lock(record_mutex);
        for (auto it = called_records.begin(); it != called_records.end();) {
            // 清理超过 5 分钟的记录
            if (current_time - it->second > 300) {
                it = called_records.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}
