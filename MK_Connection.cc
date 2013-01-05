#include "MK_Connection.h"
#include "rs232.h"

#include <iostream>

using namespace std;

enum RxCommand : uint8_t {
	RXCMD_DEBUG_HEADER = 'A',
	RXCMD_DEBUG_OUTPUT = 'D',
	RXCMD_DEBUGSM_OUTPUT = 'I'
};

enum TxCommands : uint8_t {
	TXCMD_DEBUG_HEADER = 'a',
	TXCMD_DEBUG_OUTPUT = 'd',
	TXCMD_DEBUGSM_OUTPUT = 'i',
	TXCMD_DEBUGSM_HEADER = 'o'
};

enum { TX_BUFFER_SIZE = 4096 };
enum { RX_BUFFER_SIZE = 4096 };

int mComPortId;

uint8_t mTxBufferData[TX_BUFFER_SIZE];
uint8_t mRxBufferData[RX_BUFFER_SIZE];

Buffer_t mRxBuffer;

DebugHeaderCallback mDebugHeaderCallback;
DebugOutputCallback mDebugOutputCallback;
DebugSmOutCallback mDebugSmOutCallback;

bool OpenMKConnection(int comPortId, int baudrate)
{
	if (OpenComport(comPortId, baudrate) == 0) {
		mComPortId = comPortId;
		Buffer_Init(&mRxBuffer, mRxBufferData, RX_BUFFER_SIZE);
		return false;
	}
	else {
		cerr << "Failed to open COM port #" << comPortId << " @ " << baudrate << endl;
		return true;
	}

}

void SendDebugHeaderRequest(bool lightDB)
{
	static uint8_t mHeaderLabel = 0;

	// Initialize a buffer for the packet
	Buffer_t txBuffer;
	Buffer_Init(&txBuffer, mTxBufferData, TX_BUFFER_SIZE);

	// Roll the header label
	if (lightDB)
	{
		mHeaderLabel = (++mHeaderLabel % 4);
		MKProtocol_CreateSerialFrame(&txBuffer, TXCMD_DEBUGSM_HEADER, FC_ADDRESS, 1, &mHeaderLabel, 1);
	}
	else {
		mHeaderLabel = (++mHeaderLabel % 32);
		MKProtocol_CreateSerialFrame(&txBuffer, TXCMD_DEBUG_HEADER, FC_ADDRESS, 1, &mHeaderLabel, 1);
	}

	// Send txBuffer to NaviCtrl
	SendBuffer(txBuffer);
}

void SendDebugOutputRequest(uint8_t interval, bool lightDB)
{
	// Initialize a buffer for the packet
	Buffer_t txBuffer;
	Buffer_Init(&txBuffer, mTxBufferData, TX_BUFFER_SIZE);

	if (lightDB) MKProtocol_CreateSerialFrame(&txBuffer, TXCMD_DEBUGSM_OUTPUT, FC_ADDRESS, 1, &interval, 1);
	else MKProtocol_CreateSerialFrame(&txBuffer, TXCMD_DEBUG_OUTPUT, FC_ADDRESS, 1, &interval, 1);

	// Send txBuffer to NaviCtrl
	SendBuffer(txBuffer);
}

void ProcessIncoming()
{
	const int RX_BUFFER_SIZE = 4096;
	uint8_t buffer[RX_BUFFER_SIZE];
	int readBytes = 0;

	do {
		// Read from the COM port
		readBytes = PollComport(mComPortId, buffer, RX_BUFFER_SIZE);

		// Handle the incoming data
		for (int i = 0; i < readBytes; ++i) {
			if (MKProtocol_CollectSerialFrame(&mRxBuffer, buffer[i])) {
				SerialMsg_t msg;
				MKProtocol_DecodeSerialFrameHeader(&mRxBuffer, &msg);
				MKProtocol_DecodeSerialFrameData(&mRxBuffer, &msg);

				// Reset the buffer so it can be used for next message
				Buffer_Clear(&mRxBuffer);

				if (msg.Address == FC_ADDRESS) {
					// Messages addressed from FlightCtrl
					switch (msg.CmdID) {
					case RXCMD_DEBUG_HEADER:
						HandleDebugHeader(msg);
						break;
					case RXCMD_DEBUG_OUTPUT:
						HandleDebugOutput(msg, false);
					case RXCMD_DEBUGSM_OUTPUT:
						HandleDebugOutput(msg, true);
						break;
					default:
						cerr << "Unhandled FlightCtrl command: " << msg.CmdID << endl;
						break;
					}
				} else {
					cerr << "Unhandled MK command \'" << msg.CmdID << "\' with addresses: #"
							<< (int)msg.Address << endl;
				}
			}
		}

	} while (readBytes == RX_BUFFER_SIZE);
}

void SendBuffer(const Buffer_t& txBuffer)
{
	uint16_t length = txBuffer.DataBytes;
	uint8_t* data = txBuffer.pData;

	while (length > 0) {
		int sent = SendBuf(mComPortId, data, length);
		length -= sent;
		data += sent;
	}
}

void SetDebugHeaderCallback(const DebugHeaderCallback &callback)
{
	mDebugHeaderCallback = callback;
}

void SetDebugOutputCallback(const DebugOutputCallback &callback)
{
	mDebugOutputCallback = callback;
}

void SetDebugSmOutCallback(const DebugSmOutCallback &callback)
{
	mDebugSmOutCallback = callback;
}

void HandleDebugHeader(const SerialMsg_t& msg)
{
	mDebugHeaderCallback(reinterpret_cast<const char*>(msg.pData));
}

void HandleDebugOutput(const SerialMsg_t& msg, bool lightDB)
{
	if (lightDB) {
		const DebugSmOut_t *pDebugData = reinterpret_cast<const DebugSmOut_t*>(msg.pData);
		mDebugSmOutCallback(*pDebugData);
	} else {
		const DebugOut_t *pDebugData = reinterpret_cast<const DebugOut_t*>(msg.pData);
		mDebugOutputCallback(*pDebugData);
	}
}
