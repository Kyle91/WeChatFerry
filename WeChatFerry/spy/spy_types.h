#pragma once

#include "framework.h"
#include <string>

typedef uint64_t QWORD;

struct WxString {
    const wchar_t *wptr;
    DWORD size;
    DWORD capacity;
    const char *ptr;
    DWORD clen;
    WxString()
    {
        wptr     = NULL;
        size     = 0;
        capacity = 0;
        ptr      = NULL;
        clen     = 0;
    }

    WxString(std::wstring &ws)
    {
        wptr     = ws.c_str();
        size     = (DWORD)ws.size();
        capacity = (DWORD)ws.capacity();
        ptr      = NULL;
        clen     = 0;
    }
};

struct A2Struct {
    const char* data;    // *(CHAR**)a2
    uint64_t padding1;   // 填充
    uint64_t padding2;   // 填充
    uint64_t field_16;   // *(a2 + 16)
    uint64_t field_24;   // *(a2 + 24)
};

struct PayInfo {
    char padding1[0x30];  // 填充，确保对齐
    char transaction_id[32];  // 转账 ID
    uint32_t timestamp;//时间戳
    uint32_t transaction_id_length;  // 转账 ID 长度
    char padding2[0x58 - 0x30 - 32 - sizeof(transaction_id_length)];  // 填充
    char transfer_id[31];  // 转账 ID
    uint32_t transfer_id_length;  // 转账 ID 长度
    char padding3[0x224 - 0x7C];   // 填充至0x224大小
};


typedef struct RawVector {
#ifdef _DEBUG
    QWORD head;
#endif
    QWORD start;
    QWORD finish;
    QWORD end;
} RawVector_t;
