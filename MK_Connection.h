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

// 18-byte data structure for fast transmission (up to 192 Hz)
struct HighSpeed_t {
	int16_t int16[8];
	uint8_t uint8[2];
} __attribute__((packed));

typedef std::function<void(const char*)> HeaderCallback;
typedef std::function<void(const DebugOut_t&)> DebugOutputCallback;
typedef std::function<void(const HighSpeed_t&)> HighSpeedOutputCallback;

bool OpenMKConnection(int comPortId, int baudrate);

void ProcessIncoming();

void SendHeaderRequest(bool highSpeed);
void SendOutputRequest(uint8_t interval);
void SendHighSpeedResetRequest(void);

void SendBuffer(const Buffer_t& txBuffer);

void SetHeaderCallback(const HeaderCallback &callback);
void SetDebugOutputCallback(const DebugOutputCallback &callback);
void SetHighSpeedOutputCallback(const HighSpeedOutputCallback &callback);

void HandleHeader(const SerialMsg_t& msg);
void HandleDebugOutput(const SerialMsg_t& msg);
void HandleHighSpeedOutput(const SerialMsg_t& msg);

#endif
