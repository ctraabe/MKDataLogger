#ifndef __MK_CONNECTION_H
#define __MK_CONNECTION_H

#include "MK_Protocol.h"
#include "rs232.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

struct DebugOut_t
{
	uint8_t StatusGreen;
	uint8_t StatusRed;
	int16_t Analog[32];
} __attribute__((packed));


class MKConnection {
	public:
		typedef std::function<void(const DebugOut_t&)> DebugOutputCallback;
		typedef std::function<void(const char*)> DebugHeaderCallback;

		MKConnection() : mOpen(false) {}
		MKConnection(int comPortId, int baudrate);
		~MKConnection();

		operator bool() const { return mOpen; }

		void ProcessIncoming();

		void SendDebugOutputInterval(uint8_t interval);

		void SetDebugHeaderCallback(const DebugHeaderCallback &callback) {
			mDebugHeaderCallback = callback;
		}

		void SetDebugOutputCallback(const DebugOutputCallback &callback) {
			mDebugOutputCallback = callback;
		}

	private:
		void SendBuffer(const Buffer_t& txBuffer);
		void HandleDebugHeader(const SerialMsg_t& msg);
		void HandleDebugOutput(const SerialMsg_t& msg);

		bool mOpen;
		int mComPortId;

		enum { TX_BUFFER_SIZE = 4096 };
		uint8_t mTxBufferData[TX_BUFFER_SIZE];
		enum { RX_BUFFER_SIZE = 4096 };
		uint8_t mRxBufferData[RX_BUFFER_SIZE];

		Buffer_t mRxBuffer;

		// Callbacks
		DebugHeaderCallback mDebugHeaderCallback;
		DebugOutputCallback mDebugOutputCallback;
};

#endif
