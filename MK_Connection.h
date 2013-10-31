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

// Small debug structure for fast debug reporting
struct DebugSmOut_t
{
 int16_t Analog[4];
} __attribute__((packed));

typedef std::function<void(const char*)> DebugHeaderCallback;
typedef std::function<void(const DebugOut_t&)> DebugOutputCallback;
typedef std::function<void(const DebugSmOut_t&)> DebugSmOutCallback;

bool OpenMKConnection(int comPortId, int baudrate);

void ProcessIncoming();

void SendDebugHeaderRequest(bool lightDB);
void SendDebugOutputRequest(uint8_t interval, bool lightDB);
void SendMotorTestRequest(uint8_t setPoint);

void SendBuffer(const Buffer_t& txBuffer);

void SetDebugHeaderCallback(const DebugHeaderCallback &callback);
void SetDebugOutputCallback(const DebugOutputCallback &callback);
void SetDebugSmOutCallback(const DebugSmOutCallback &callback);

void HandleDebugHeader(const SerialMsg_t& msg);
void HandleDebugOutput(const SerialMsg_t& msg, bool lightDB);

#endif
