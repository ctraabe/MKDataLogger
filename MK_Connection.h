#ifndef __MKCONNECTION_H
#define __MKCONNECTION_H

#include "MK_Protocol.h"

#include <functional>

struct DebugOut_t
{
	uint8_t StatusGreen;
	uint8_t StatusRed;
	int16_t Analog[32];
} __attribute__((packed));

typedef std::function<void(const char*)> DebugHeaderCallback;
typedef std::function<void(const DebugOut_t&)> DebugOutputCallback;

bool OpenMKConnection(int comPortId, int baudrate);

void ProcessIncoming();

void SendDebugHeaderRequest();
void SendDebugOutputRequest(uint8_t interval);

void SendBuffer(const Buffer_t& txBuffer);

void SetDebugHeaderCallback(const DebugHeaderCallback &callback);
void SetDebugOutputCallback(const DebugOutputCallback &callback);

void HandleDebugHeader(const SerialMsg_t& msg);
void HandleDebugOutput(const SerialMsg_t& msg);

#endif
