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
MsgTypes_t GetMsgTypes();
