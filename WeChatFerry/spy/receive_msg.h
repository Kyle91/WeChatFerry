#pragma once

#include "pb_types.h"

void EnableLog();
void DisableLog();
void ListenPyq();
void UnListenPyq();
void ListenMessage();
void UnListenMessage();
void ListenMemberUpdate();
void UnListenMemberUpdate();
void ListenQrPayment();
void UnListenQrPayment();
void ListenMemberBeKick();
void UnListenMemberBeKick();
MsgTypes_t GetMsgTypes();

// 定义群成员模型
struct ModelGroupMember {
    std::string GroupAlias;
    std::string Wxid;
    std::string Gid;
    std::string Name;
};

