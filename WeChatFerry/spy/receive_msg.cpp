﻿#pragma execution_character_set("utf-8")

#include "MinHook.h"
#include "framework.h"
#include <condition_variable>
#include <mutex>
#include <queue>

#include "log.h"
#include "receive_msg.h"
#include "user_info.h"
#include "util.h"
#include "rpc_server.h"
#include "member_add_mgmt.h"
#include "member_quit_mgmt.h"
#include "exec_sql.h"
#include "wcf.pb.h"
#include "pb_util.h"
#include <unordered_set>

// Defined in rpc_server.cpp
extern bool gIsLogging, gIsListening, gIsListeningPyq, gIsListenMemberUpdate, gIsListenQrPayment, gIsListenKickMember;
extern mutex gMutex;
extern condition_variable gCV;
extern queue<WxMsg_t> gMsgQueue;

// Defined in spy.cpp
extern QWORD g_WeChatWinDllAddr;

#define OS_RECV_MSG_ID      0x30
#define OS_RECV_MSG_TYPE    0x38
#define OS_RECV_MSG_SELF    0x3C
#define OS_RECV_MSG_TS      0x44
#define OS_RECV_MSG_ROOMID  0x48
#define OS_RECV_MSG_CONTENT 0x88
#define OS_RECV_MSG_WXID    0x240
#define OS_RECV_MSG_SIGN    0x260
#define OS_RECV_MSG_THUMB   0x280
#define OS_RECV_MSG_EXTRA   0x2A0
#define OS_RECV_MSG_XML     0x308
#define OS_RECV_MSG_CALL    0x213ED90
#define OS_PYQ_MSG_START    0x30
#define OS_PYQ_MSG_END      0x38
#define OS_PYQ_MSG_TS       0x38
#define OS_PYQ_MSG_XML      0x9B8
#define OS_PYQ_MSG_SENDER   0x18
#define OS_PYQ_MSG_CONTENT  0x48
#define OS_PYQ_MSG_CALL     0x2E42C90
#define OS_WXLOG            0x2613D20
#define OS_MEMBER_UPDATE    0x2162bc0
#define OS_QR_PAYMENT       0x1e957d0
#define OS_MEMBER_BE_KICK   0x215a710

typedef QWORD (*RecvMsg_t)(QWORD, QWORD);
typedef QWORD (*WxLog_t)(QWORD, QWORD, QWORD, QWORD, QWORD, QWORD, QWORD, QWORD, QWORD, QWORD, QWORD, QWORD);
typedef QWORD (*RecvPyq_t)(QWORD, QWORD, QWORD);
typedef QWORD (*MemberUpdate_t)(__int64 a1, __int64 a2, int a3, __int64 a4, __int64 a5, __int64 a6, QWORD* a7);
typedef QWORD (*QrPayment_t)(__int64 a1, __int64 a2, __int64 a3);
typedef QWORD (*MemberBeKick_t)(__int64 a1, __int64 a2, char** a3, char a4);

static RecvMsg_t funcRecvMsg = nullptr;
static RecvMsg_t realRecvMsg = nullptr;
static WxLog_t funcWxLog     = nullptr;
static WxLog_t realWxLog     = nullptr;
static RecvPyq_t funcRecvPyq = nullptr;
static RecvPyq_t realRecvPyq = nullptr;
static MemberUpdate_t funcMemberUpdate = nullptr;
static MemberUpdate_t realMemberUpdate = nullptr;
static QrPayment_t funcQrPayment = nullptr;
static QrPayment_t realQrPayment = nullptr;
static MemberBeKick_t funcMemberBeKick = nullptr;
static MemberBeKick_t realMemberBeKick = nullptr;
static bool isMH_Initialized = false;

MsgTypes_t GetMsgTypes()
{
    const MsgTypes_t m = {
        { 0x00, "朋友圈消息" },
        { 0x01, "文字" },
        { 0x03, "图片" },
        { 0x22, "语音" },
        { 0x25, "好友确认" },
        { 0x28, "POSSIBLEFRIEND_MSG" },
        { 0x2A, "名片" },
        { 0x2B, "视频" },
        { 0x2F, "石头剪刀布 | 表情图片" },
        { 0x30, "位置" },
        { 0x31, "共享实时位置、文件、转账、链接" },
        { 0x32, "VOIPMSG" },
        { 0x33, "微信初始化" },
        { 0x34, "VOIPNOTIFY" },
        { 0x35, "VOIPINVITE" },
        { 0x3E, "小视频" },
        { 0x42, "微信红包" },
        { 0x270F, "SYSNOTICE" },
        { 0x2710, "红包、系统消息" },
        { 0x2712, "撤回消息" },
        { 0x100031, "搜狗表情" },
        { 0x1000031, "链接" },
        { 0x1A000031, "微信红包" },
        { 0x20010031, "红包封面" },
        { 0x2D000031, "视频号视频" },
        { 0x2E000031, "视频号名片" },
        { 0x31000031, "引用消息" },
        { 0x37000031, "拍一拍" },
        { 0x3A000031, "视频号直播" },
        { 0x3A100031, "商品链接" },
        { 0x3A200031, "视频号直播" },
        { 0x3E000031, "音乐链接" },
        { 0x41000031, "文件" },
    };

    return m;
}


static QWORD DispatchMsg(QWORD arg1, QWORD arg2)
{
    WxMsg_t wxMsg = { 0 };
    try {
        wxMsg.id      = GET_QWORD(arg2 + OS_RECV_MSG_ID);
        wxMsg.type    = GET_DWORD(arg2 + OS_RECV_MSG_TYPE);
        wxMsg.is_self = GET_DWORD(arg2 + OS_RECV_MSG_SELF);
        wxMsg.ts      = GET_DWORD(arg2 + OS_RECV_MSG_TS);
        wxMsg.content = GetStringByWstrAddr(arg2 + OS_RECV_MSG_CONTENT);
        wxMsg.sign    = GetStringByWstrAddr(arg2 + OS_RECV_MSG_SIGN);
        wxMsg.xml     = GetStringByWstrAddr(arg2 + OS_RECV_MSG_XML);

        string roomid = GetStringByWstrAddr(arg2 + OS_RECV_MSG_ROOMID);
        wxMsg.roomid  = roomid;
        if (roomid.find("@chatroom") != string::npos) { // 群 ID 的格式为 xxxxxxxxxxx@chatroom
            wxMsg.is_group = true;
            if (wxMsg.is_self) {
                wxMsg.sender = GetSelfWxid();
            } else {
                wxMsg.sender = GetStringByWstrAddr(arg2 + OS_RECV_MSG_WXID);
            }
        } else {
            wxMsg.is_group = false;
            if (wxMsg.is_self) {
                wxMsg.sender = GetSelfWxid();
            } else {
                wxMsg.sender = roomid;
            }
        }

        wxMsg.thumb = GetStringByWstrAddr(arg2 + OS_RECV_MSG_THUMB);
        if (!wxMsg.thumb.empty()) {
            wxMsg.thumb = GetHomePath() + wxMsg.thumb;
            replace(wxMsg.thumb.begin(), wxMsg.thumb.end(), '\\', '/');
        }

        wxMsg.extra = GetStringByWstrAddr(arg2 + OS_RECV_MSG_EXTRA);
        if (!wxMsg.extra.empty()) {
            wxMsg.extra = GetHomePath() + wxMsg.extra;
            replace(wxMsg.extra.begin(), wxMsg.extra.end(), '\\', '/');
        }

        if (wxMsg.type == 10000) {
            if (wxMsg.content.find("加入了群聊") != std::string::npos) {
                auto& member_mgmt = MemberAddMgmt::GetInstance();
                member_mgmt.AddSystemMessage(wxMsg.roomid, wxMsg.content);
            }
        }
    } catch (const std::exception &e) {
        LOG_ERROR(GB2312ToUtf8(e.what()));
    } catch (...) {
        LOG_ERROR("Unknow exception.");
    }

    {
        unique_lock<mutex> lock(gMutex);
        gMsgQueue.push(wxMsg); // 推送到队列
    }

    gCV.notify_all(); // 通知各方消息就绪
    return realRecvMsg(arg1, arg2);
}

static QWORD PrintWxLog(QWORD a1, QWORD a2, QWORD a3, QWORD a4, QWORD a5, QWORD a6, QWORD a7, QWORD a8, QWORD a9,
                        QWORD a10, QWORD a11, QWORD a12)
{
    QWORD p = realWxLog(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    if (p == 0 || p == 1) {
        return p;
    }

    LOG_INFO("【WX】\n{}", GB2312ToUtf8((char *)p));

    return p;
}

static void DispatchPyq(QWORD arg1, QWORD arg2, QWORD arg3)
{
    QWORD startAddr = *(QWORD *)(arg2 + OS_PYQ_MSG_START);
    QWORD endAddr   = *(QWORD *)(arg2 + OS_PYQ_MSG_END);

    if (startAddr == 0) {
        return;
    }

    while (startAddr < endAddr) {
        WxMsg_t wxMsg;

        wxMsg.type     = 0x00; // 朋友圈消息
        wxMsg.is_self  = false;
        wxMsg.is_group = false;
        wxMsg.id       = GET_QWORD(startAddr);
        wxMsg.ts       = GET_DWORD(startAddr + OS_PYQ_MSG_TS);
        wxMsg.xml      = GetStringByWstrAddr(startAddr + OS_PYQ_MSG_XML);
        wxMsg.sender   = GetStringByWstrAddr(startAddr + OS_PYQ_MSG_SENDER);
        wxMsg.content  = GetStringByWstrAddr(startAddr + OS_PYQ_MSG_CONTENT);

        {
            unique_lock<mutex> lock(gMutex);
            gMsgQueue.push(wxMsg); // 推送到队列
        }

        gCV.notify_all(); // 通知各方消息就绪

        startAddr += 0x1618;
    }
}

static MH_STATUS InitializeHook()
{
    if (isMH_Initialized) {
        return MH_OK;
    }
    MH_STATUS status = MH_Initialize();
    if (status == MH_OK) {
        isMH_Initialized = true;
    }
    return status;
}

static MH_STATUS UninitializeHook()
{
    if (!isMH_Initialized) {
        return MH_OK;
    }
    if (gIsLogging || gIsListening || gIsListeningPyq) {
        return MH_OK;
    }
    MH_STATUS status = MH_Uninitialize();
    if (status == MH_OK) {
        isMH_Initialized = false;
    }
    return status;
}

void EnableLog()
{
    MH_STATUS status = MH_UNKNOWN;
    if (gIsLogging) {
        LOG_WARN("gIsLogging");
        return;
    }
    WxLog_t funcWxLog = (WxLog_t)(g_WeChatWinDllAddr + OS_WXLOG);

    status = InitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Initialize failed: {}", to_string(status));
        return;
    }

    status = MH_CreateHook(funcWxLog, &PrintWxLog, reinterpret_cast<LPVOID *>(&realWxLog));
    if (status != MH_OK) {
        LOG_ERROR("MH_CreateHook failed: {}", to_string(status));
        return;
    }

    status = MH_EnableHook(funcWxLog);
    if (status != MH_OK) {
        LOG_ERROR("MH_EnableHook failed: {}", to_string(status));
        return;
    }
    gIsLogging = true;
}

void DisableLog()
{
    MH_STATUS status = MH_UNKNOWN;
    if (!gIsLogging) {
        return;
    }

    status = MH_DisableHook(funcWxLog);
    if (status != MH_OK) {
        LOG_ERROR("MH_DisableHook failed: {}", to_string(status));
        return;
    }

    gIsLogging = false;

    status = UninitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Uninitialize failed: {}", to_string(status));
        return;
    }
}

void ListenMessage()
{
    MH_STATUS status = MH_UNKNOWN;
    if (gIsListening) {
        LOG_WARN("gIsListening");
        return;
    }
    funcRecvMsg = (RecvMsg_t)(g_WeChatWinDllAddr + OS_RECV_MSG_CALL);

    status = InitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Initialize failed: {}", to_string(status));
        return;
    }

    status = MH_CreateHook(funcRecvMsg, &DispatchMsg, reinterpret_cast<LPVOID *>(&realRecvMsg));
    if (status != MH_OK) {
        LOG_ERROR("MH_CreateHook failed: {}", to_string(status));
        return;
    }

    status = MH_EnableHook(funcRecvMsg);
    if (status != MH_OK) {
        LOG_ERROR("MH_EnableHook failed: {}", to_string(status));
        return;
    }

    gIsListening = true;
}

void UnListenMessage()
{
    MH_STATUS status = MH_UNKNOWN;
    if (!gIsListening) {
        return;
    }

    status = MH_DisableHook(funcRecvMsg);
    if (status != MH_OK) {
        LOG_ERROR("MH_DisableHook failed: {}", to_string(status));
        return;
    }

    gIsListening = false;

    status = UninitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Uninitialize failed: {}", to_string(status));
        return;
    }
}

void ListenPyq()
{
    MH_STATUS status = MH_UNKNOWN;
    if (gIsListeningPyq) {
        LOG_WARN("gIsListeningPyq");
        return;
    }
    funcRecvPyq = (RecvPyq_t)(g_WeChatWinDllAddr + OS_PYQ_MSG_CALL);

    status = InitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Initialize failed: {}", to_string(status));
        return;
    }

    status = MH_CreateHook(funcRecvPyq, &DispatchPyq, reinterpret_cast<LPVOID *>(&realRecvPyq));
    if (status != MH_OK) {
        LOG_ERROR("MH_CreateHook failed: {}", to_string(status));
        return;
    }

    status = MH_EnableHook(funcRecvPyq);
    if (status != MH_OK) {
        LOG_ERROR("MH_EnableHook failed: {}", to_string(status));
        return;
    }

    gIsListeningPyq = true;
}

void UnListenPyq()
{
    MH_STATUS status = MH_UNKNOWN;
    if (!gIsListeningPyq) {
        return;
    }

    status = MH_DisableHook(funcRecvPyq);
    if (status != MH_OK) {
        LOG_ERROR("MH_DisableHook failed: {}", to_string(status));
        return;
    }

    gIsListeningPyq = false;

    status = UninitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Uninitialize failed: {}", to_string(status));
        return;
    }
}

std::string GetColumnContent(const DbRow_t& row, const std::string& columnName) {
    for (const auto& field : row) {
        if (field.column == columnName) {
            return std::string(field.content.begin(), field.content.end());
        }
    }
    return ""; // 没有找到返回空字符串
}

// 获取群成员信息
std::vector<ModelGroupMember> GetRoomMembers(const std::string& gid) {
    std::vector<ModelGroupMember> result;

    // 查询群信息
    std::string query = "SELECT RoomData FROM ChatRoom WHERE ChatRoomName = '" + gid + "';";
    auto roomList = ExecDbQuery("MicroMsg.db", query);

    if (roomList.empty()) {
        return result;
    }

    std::string roomDataStr = GetColumnContent(roomList[0], "RoomData");
    if (roomDataStr.empty()) {
        return result;
    }

    // Protobuf解析
    pb_istream_t stream = pb_istream_from_buffer(reinterpret_cast<const uint8_t*>(roomDataStr.data()), roomDataStr.size());

    // 创建 RoomData 结构实例
    RoomData roomData = RoomData_init_zero; // 初始化为零值

    // 解码数据到 RoomData
    if (!pb_decode(&stream, RoomData_fields, &roomData)) {
        LOG_ERROR("Failed to decode RoomData protobuf.");
        return result;
    }
    // 遍历成员
    for (int i = 0; i < roomData.members_count; ++i) {
        const auto& member = roomData.members[i];
        ModelGroupMember groupMember;
        groupMember.GroupAlias = member.name;
        groupMember.Wxid = member.wxid;
        groupMember.Gid = gid;
        result.push_back(groupMember);
    }

    // 批量获取成员昵称
    std::vector<std::string> wxids;
    for (const auto& member : result) {
        wxids.push_back("'" + member.Wxid + "'");
    }
    std::string wxidList = Join(wxids, ",");
    query = "SELECT UserName, NickName FROM Contact WHERE UserName IN (" + wxidList + ");";
    auto userNicknames = ExecDbQuery("MicroMsg.db", query);

    // 映射昵称
    std::map<std::string, std::string> nicknameDict;
    for (const auto& row : userNicknames) {
        std::string userName = GetColumnContent(row, "UserName");
        std::string nickName = GetColumnContent(row, "NickName");
        nicknameDict[userName] = nickName;
    }

    // 设置昵称
    for (auto& member : result) {
        member.Name = nicknameDict[member.Wxid];
    }

    return result;
}

MemberEntry ParseMemberInfo(__int64 memberAddr) {
    MemberEntry member = {};
    try {
        uint32_t mask = *reinterpret_cast<uint32_t*>(memberAddr + 92);

        if (mask & 1) {
            __int64 field1Ptr = *reinterpret_cast<__int64*>(memberAddr + 8);
            if (field1Ptr != 0) {
                member.member_id = ReadAdjustedString(field1Ptr);
            }
        }

        if (mask & 2) {
            __int64 field2Ptr = *reinterpret_cast<__int64*>(memberAddr + 16);
            if (field2Ptr != 0) {
                member.member_nickname = ReadAdjustedString(field2Ptr);
            }
        }

        /*if (mask & 8) {
            __int64 field3Ptr = *reinterpret_cast<__int64*>(memberAddr + 32);
            if (field3Ptr != 0) {
                string hd = ReadAdjustedString(field3Ptr);
                LOG_INFO("ParseMemberInfo hd {}", hd);
                member.avatarHd = _strdup(hd.c_str());
            }
        }

        if (mask & 0x10) {
            __int64 field4Ptr = *reinterpret_cast<__int64*>(memberAddr + 40);
            if (field4Ptr != 0) {
                string hd = ReadAdjustedString(field4Ptr);
                LOG_INFO("ParseMemberInfo avatar {}", hd);
                member.avatar = _strdup(hd.c_str());
            }
        }*/

        if (mask & 0x40) {
            __int64 field5Ptr = *reinterpret_cast<__int64*>(memberAddr + 48);
            if (field5Ptr != 0) {
                member.inviter = ReadAdjustedString(field5Ptr);
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error parsing member info: {}", e.what());
        throw; // 如果需要让上层处理错误，可以抛出异常
    }
    catch (...) {
        LOG_ERROR("Unknown error while parsing member info.");
        throw;
    }
    return member;
}



static QWORD ProcessMemberUpdate(__int64 a1, __int64 a2, int a3, __int64 a4, __int64 a5, __int64 a6, QWORD* a7) {
    try {
        auto& member_mgmt = MemberAddMgmt::GetInstance();
        std::string groupID = ReadUtf16String(*reinterpret_cast<__int64*>(a2));

        int memberUpdateFlag = *reinterpret_cast<int*>(a4 + 32);
        if (memberUpdateFlag > 0) {
            uint32_t arrayLength = *reinterpret_cast<uint32_t*>(a4 + 32);
            if (arrayLength > 0 && arrayLength < 10) { // Add a reasonable limit
                __int64 arrayBaseAddr = *reinterpret_cast<__int64*>(a4 + 8);
                for (uint32_t i = 0; i < arrayLength; ++i) {
                    __int64 memberAddr = *reinterpret_cast<__int64*>(arrayBaseAddr + i * 8);
                    MemberEntry member = ParseMemberInfo(memberAddr);
                    member_mgmt.AddMemberEntry(groupID,member);
                }
            }
        }
        else {
            auto& member_quit = MemberQuitMgmt::GetInstance();
            std::unordered_set<std::string> newMemberList;

            // 读取 startPtr 和 endPtr
            void* startPtr = *reinterpret_cast<void**>(a7);                        // 相当于 *(void**)a7
            void* endPtr = *reinterpret_cast<void**>(reinterpret_cast<char*>(a7) + sizeof(void*)); // *(a7 + 1)

            // 定义每个元素的大小（32字节）
            const size_t elementSize = 32;

            // 初始化当前指针为起始指针
            char* currentPtr = static_cast<char*>(startPtr);

            // 遍历数组
            while (currentPtr < static_cast<char*>(endPtr)) {
                // 读取当前元素的第一个字段作为成员ID指针
                void* memberIdPtr = *reinterpret_cast<void**>(currentPtr); // 读取第一个字段
                const char* memberId = static_cast<const char*>(memberIdPtr);

                // 检查 memberId 是否为有效指针，避免非法访问
                if (memberId != nullptr) {
                    // 保存到结果数组中
                    newMemberList.insert(std::string(memberId));
                }
                // 移动到下一个元素
                currentPtr += elementSize;
            }
            std::vector<ModelGroupMember> oldMemberList = GetRoomMembers(groupID);

            std::vector<ModelGroupMember> removedMembers;

            // 遍历旧成员列表，检查是否存在于新成员集合中
            for (const auto& oldMember : oldMemberList) {
                if (newMemberList.find(oldMember.Wxid) == newMemberList.end()) {
                    // 如果不在新成员集合中，将其添加到结果列表
                    removedMembers.push_back(oldMember);
                }
            }
            if (removedMembers.size() > 0) {
                member_quit.AddMemberQuit(groupID, removedMembers);
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error processing member update: {}", e.what());
        return realMemberUpdate(a1, a2, a3, a4, a5, a6, a7);
    }
    catch (...) {
        LOG_ERROR("Unknown error processing member update.");
        return realMemberUpdate(a1, a2, a3, a4, a5, a6, a7);
    }
    return realMemberUpdate(a1, a2, a3, a4, a5, a6, a7);

}

// 这里是群成员信息变动
void ListenMemberUpdate()
{
    MH_STATUS status = MH_UNKNOWN;
    if (gIsListenMemberUpdate) {
        LOG_WARN("gIsListenMemberUpdate");
        return;
    }
    MemberUpdate_t funcMemberUpdate = (MemberUpdate_t)(g_WeChatWinDllAddr + OS_MEMBER_UPDATE);

    status = InitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Initialize failed: {}", to_string(status));
        return;
    }

    LOG_INFO("create member update hook");

    status = MH_CreateHook(funcMemberUpdate, &ProcessMemberUpdate, reinterpret_cast<LPVOID*>(&realMemberUpdate));
    if (status != MH_OK) {
        LOG_ERROR("MH_CreateHook failed: {}", to_string(status));
        return;
    }

    status = MH_EnableHook(funcMemberUpdate);
    if (status != MH_OK) {
        LOG_ERROR("MH_EnableHook failed: {}", to_string(status));
        return;
    }
    gIsListenMemberUpdate = true;
}

void UnListenMemberUpdate()
{
    MH_STATUS status = MH_UNKNOWN;
    if (!gIsListenMemberUpdate) {
        return;
    }

    status = MH_DisableHook(funcMemberUpdate);
    if (status != MH_OK) {
        LOG_ERROR("MH_DisableHook failed: {}", to_string(status));
        return;
    }

    gIsListenMemberUpdate = false;

    status = UninitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Uninitialize failed: {}", to_string(status));
        return;
    }
}

static QWORD ProcessQrPayment(__int64 a1, __int64 a2, __int64 a3) {
    __int64 stringAddress = *reinterpret_cast<__int64*>(a2);
    std::string payStr = ReadUtf16String(stringAddress);
    WxMsg_t wxMsg = { 0 };
    wxMsg.type = 20002;
    wxMsg.is_self = false;
    wxMsg.ts = GetCurrentTimestamp();
    wxMsg.is_group = false;
    wxMsg.xml = payStr;
    {
        lock_guard<mutex> lock(gMutex);
        gMsgQueue.push(wxMsg);
    }
    return realQrPayment(a1, a2, a3);
}

//二维码收款信息
void ListenQrPayment()
{
    MH_STATUS status = MH_UNKNOWN;
    if (gIsListenQrPayment) {
        LOG_WARN("gIsListenQrPayment");
        return;
    }
    QrPayment_t funcQrPayment = (QrPayment_t)(g_WeChatWinDllAddr + OS_QR_PAYMENT);

    status = InitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Initialize failed: {}", to_string(status));
        return;
    }

    LOG_INFO("create payment update hook");

    status = MH_CreateHook(funcQrPayment, &ProcessQrPayment, reinterpret_cast<LPVOID*>(&realQrPayment));
    if (status != MH_OK) {
        LOG_ERROR("MH_CreateHook failed: {}", to_string(status));
        return;
    }

    status = MH_EnableHook(funcQrPayment);
    if (status != MH_OK) {
        LOG_ERROR("MH_EnableHook failed: {}", to_string(status));
        return;
    }
    gIsListenQrPayment = true;
}

void UnListenQrPayment()
{
    MH_STATUS status = MH_UNKNOWN;
    if (!gIsListenQrPayment) {
        return;
    }

    status = MH_DisableHook(funcQrPayment);
    if (status != MH_OK) {
        LOG_ERROR("MH_DisableHook failed: {}", to_string(status));
        return;
    }

    gIsListenQrPayment = false;

    status = UninitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Uninitialize failed: {}", to_string(status));
        return;
    }
}

static QWORD ProcessMemberBeKick(__int64 a1, __int64 a2, char** a3, char a4) {
    std::string groupID = ReadUtf16String(a1);
    try {
        // 读取 a2 + 8 位置的结束指针
        void* endPtr = *reinterpret_cast<void**>(reinterpret_cast<char*>(a2) + 0x8);
        // 读取 a2 位置的起始指针
        void* currentPtr = *reinterpret_cast<void**>(a2);

        std::unordered_set<std::string> wxidSet;

        int count = 0;
        // 遍历列表，直到 currentPtr == endPtr
        while (currentPtr != endPtr) {
            count++;
            if (count % 8 == 1) {
                // 读取当前 wxid 的指针
                void* wxidPtr = *reinterpret_cast<void**>(currentPtr);
                std::string wxid = ReadUtf8String(wxidPtr);
                wxidSet.insert(wxid);
            }
            // 偏移到下一个条目，假设每个条目占 8 字节
            currentPtr = reinterpret_cast<char*>(currentPtr) + 0x4;
        }

        auto& member_quit = MemberQuitMgmt::GetInstance();
        std::vector<ModelGroupMember> oldMemberList = GetRoomMembers(groupID);
        std::vector<ModelGroupMember> removedMembers;
        for (const auto& oldMember : oldMemberList) {
            if (wxidSet.find(oldMember.Wxid) != wxidSet.end()) {
                removedMembers.push_back(oldMember);
            }
        }
        if (removedMembers.size() > 0) {
            member_quit.AddMemberQuit(groupID, removedMembers);
        }

    }
    catch (const std::exception& e) {
        LOG_INFO("Error in ProcessMemberBeKick: {}", e.what());
        return realMemberBeKick(a1, a2, a3, a4);
    }
    return realMemberBeKick(a1,a2,a3,a4);
}

//主动踢人
void ListenMemberBeKick()
{
    MH_STATUS status = MH_UNKNOWN;
    if (gIsListenKickMember) {
        LOG_WARN("gIsListenKickMember");
        return;
    }
    MemberBeKick_t funcMemberBeKick = (MemberBeKick_t)(g_WeChatWinDllAddr + OS_MEMBER_BE_KICK);

    status = InitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Initialize failed: {}", to_string(status));
        return;
    }

    LOG_INFO("create member be kick hook");

    status = MH_CreateHook(funcMemberBeKick, &ProcessMemberBeKick, reinterpret_cast<LPVOID*>(&realMemberBeKick));
    if (status != MH_OK) {
        LOG_ERROR("MH_CreateHook failed: {}", to_string(status));
        return;
    }

    status = MH_EnableHook(funcMemberBeKick);
    if (status != MH_OK) {
        LOG_ERROR("MH_EnableHook failed: {}", to_string(status));
        return;
    }
    gIsListenKickMember = true;
}

void UnListenMemberBeKick()
{
    MH_STATUS status = MH_UNKNOWN;
    if (!gIsListenKickMember) {
        return;
    }

    status = MH_DisableHook(funcMemberBeKick);
    if (status != MH_OK) {
        LOG_ERROR("MH_DisableHook failed: {}", to_string(status));
        return;
    }

    gIsListenKickMember = false;

    status = UninitializeHook();
    if (status != MH_OK) {
        LOG_ERROR("MH_Uninitialize failed: {}", to_string(status));
        return;
    }
}
