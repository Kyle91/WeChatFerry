#pragma once

#include "pb_types.h"

void EnableLog();
void DisableLog();
void ListenPyq();
void UnListenPyq();
void ListenMessage();
void UnListenMessage();
void ListenLoginQrCode();
void UnListenLoginQrCode();
MsgTypes_t GetMsgTypes();
