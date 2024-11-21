#include "member_add_mgmt.h"
#include <algorithm>
#include <thread>
#include <iostream>

// 获取单例实例
MemberAddMgmt& MemberAddMgmt::GetInstance() {
    static MemberAddMgmt instance;
    return instance;
}

// 私有构造函数
MemberAddMgmt::MemberAddMgmt() {}


uint32_t GetCurrentTimestamp() {
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count());
}
// 设置通知回调
void MemberAddMgmt::SetNotificationCallback(std::function<void(const std::string& gid, const MemberEntry&)> callback) {
    notification_callback = callback;
}

// 添加成员信息并匹配
void MemberAddMgmt::AddMemberEntry(const std::string& gid, MemberEntry& entry) {
    std::lock_guard<std::mutex> lock(map_mutex);
    entry.timestamp = GetCurrentTimestamp();
    group_map[gid].push_back(entry);

    // 尝试匹配系统消息
    MatchAndNotify(gid, entry);
}

// 添加系统消息并匹配
void MemberAddMgmt::AddSystemMessage(const std::string& gid, const std::string& message_content) {
    std::lock_guard<std::mutex> lock(map_mutex);
    SystemMessage sys_msg = { gid, message_content, GetCurrentTimestamp()};
    system_messages.push_back(sys_msg);

    // 尝试匹配群成员
    MatchSystemMessageAndNotify(sys_msg);
}

// 启动定时清理任务
void MemberAddMgmt::StartCleanupTask(uint32_t expiration_duration) {
    std::thread([this, expiration_duration]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒检查一次
            CleanupExpiredEntries(expiration_duration);
        }
        }).detach();
}

// 匹配成员消息与系统消息
void MemberAddMgmt::MatchAndNotify(const std::string& gid, const MemberEntry& entry) {
    uint32_t current_time = GetCurrentTimestamp();
    uint32_t notification_threshold = 10; // 设置通知的时间间隔，单位秒
    // 筛选出与 gid 匹配的系统消息
    auto it = std::find_if(system_messages.begin(), system_messages.end(),
        [&](const SystemMessage& msg) {
            return msg.group_id == gid && msg.message_content.find(entry.member_nickname) != std::string::npos;
        });

    // 如果找到匹配的系统消息
    if (it != system_messages.end()) {
        // 匹配成功，通知
        if (ShouldNotify(gid, entry.member_id, current_time, notification_threshold)) { // 检查是否需要通知
            if (notification_callback) {
                notification_callback(gid, entry);
            }
        }
    }
}


// 匹配系统消息与群成员
void MemberAddMgmt::MatchSystemMessageAndNotify(const SystemMessage& sys_msg) {
    uint32_t current_time = GetCurrentTimestamp();
    uint32_t notification_threshold = 10; // 设置通知的时间间隔，单位秒
    // 找到对应的群组
    auto it = group_map.find(sys_msg.group_id);
    if (it != group_map.end()) {
        auto& member_list = it->second;
        // 遍历成员列表
        for (const auto& member : member_list) {
            // 在系统消息中查找成员的昵称
            if (sys_msg.message_content.find(member.member_nickname) != std::string::npos) {
               if (ShouldNotify(sys_msg.group_id, member.member_id, current_time, notification_threshold)) {
                    // 匹配成功，通知
                    if (notification_callback) {
                        notification_callback(sys_msg.group_id, member);
                    }
                }
              
            }
        }
    }
}

bool MemberAddMgmt::ShouldNotify(const std::string& gid, const std::string& member_id, uint32_t current_time, uint32_t threshold) {
    std::lock_guard<std::mutex> lock(notified_mutex); // 使用独立的锁

    auto group_it = notified_entries.find(gid);
    if (group_it != notified_entries.end()) {
        auto& member_map = group_it->second;
        auto member_it = member_map.find(member_id);
        if (member_it != member_map.end()) {
            if (current_time - member_it->second < threshold) {
                return false; // 短时间内已通知过
            }
        }
    }
    // 更新通知时间
    notified_entries[gid][member_id] = current_time;
    return true;
}

void MemberAddMgmt::MatchAllEntries() {
    //uint32_t current_time = GetCurrentTimestamp();
    //uint32_t notification_threshold = 10; // 设置通知的时间间隔，单位秒
    //// 遍历所有系统消息
    //for (const auto& sys_msg : system_messages) {
    //    auto group_it = group_map.find(sys_msg.group_id);
    //    if (group_it != group_map.end()) {
    //        auto& member_list = group_it->second;

    //        // 遍历群成员列表
    //        for (const auto& member : member_list) {
    //            if (sys_msg.message_content.find(member.member_nickname) != std::string::npos) {
    //                if (ShouldNotify(sys_msg.group_id, member.member_id, current_time, notification_threshold)) {
    //                    // 匹配成功，通知
    //                    if (notification_callback) {
    //                        notification_callback(sys_msg.group_id, member);
    //                    }
    //                }
    //               
    //            }
    //        }
    //    }
    //}
}


// 清理过期数据
void MemberAddMgmt::CleanupExpiredEntries(uint32_t expiration_duration) {
    uint32_t now = GetCurrentTimestamp(); // 获取当前时间戳（秒级）
    std::lock_guard<std::mutex> lock(map_mutex);

    // 清理过期的群成员
    for (auto it = group_map.begin(); it != group_map.end();) {
        auto& member_list = it->second;

        member_list.erase(std::remove_if(member_list.begin(), member_list.end(),
            [&](const MemberEntry& entry) {
                return (now - entry.timestamp) > expiration_duration; // 比较是否过期
            }),
            member_list.end());

        if (member_list.empty()) {
            it = group_map.erase(it); // 删除空的群组
        }
        else {
            ++it;
        }
    }

    // 清理过期的系统消息
    system_messages.erase(std::remove_if(system_messages.begin(), system_messages.end(),
        [&](const SystemMessage& msg) {
            return (now - msg.timestamp) > expiration_duration; // 比较是否过期
        }),
        system_messages.end());

    // 清理过期的通知记录
    for (auto it = notified_entries.begin(); it != notified_entries.end();) {
        auto& member_map = it->second;
        for (auto member_it = member_map.begin(); member_it != member_map.end();) {
            if ((now - member_it->second) > expiration_duration) {
                member_it = member_map.erase(member_it); // 删除过期记录
            }
            else {
                ++member_it;
            }
        }
        if (member_map.empty()) {
            it = notified_entries.erase(it);
        }
        else {
            ++it;
        }
    }

    //匹配数据
    //MatchAllEntries();
}

