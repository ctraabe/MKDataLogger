#include "MK_Connection.h"

using namespace std;

enum RxCommand : uint8_t {
	RXCMD_DEBUG_HEADER = 'A',
	RXCMD_DEBUG_OUTPUT = 'D'
};

enum TxCommands : uint8_t {
	TXCMD_DEBUG_HEADER = 'a',
	TXCMD_DEBUG_OUTPUT = 'd'
};

MKConnection::MKConnection(int comPortId, int baudrate)
	: mOpen(false)
	, mComPortId(comPortId)
{
	if (OpenComport(comPortId, baudrate) == 0) {
		mOpen = true;
	}

	if (!mOpen) {
		cerr << "Failed to open COM port #" << comPortId << " @ " << baudrate << endl;
	}

	Buffer_Init(&mRxBuffer, mRxBufferData, RX_BUFFER_SIZE);
}

MKConnection::~MKConnection() {
}

void MKConnection::SendDebugOutputInterval(uint8_t interval)
{
	assert(mOpen);

	// Initialize a buffer for the packet
	Buffer_t txBuffer;
	Buffer_Init(&txBuffer, mTxBufferData, TX_BUFFER_SIZE);

	// Build the packet
	uint8_t data[1] = { interval };

	MKProtocol_CreateSerialFrame(&txBuffer, TXCMD_DEBUG_OUTPUT, FC_ADDRESS, 1, data, 1);

	// Send txBuffer to NaviCtrl
	SendBuffer(txBuffer);
}

void MKConnection::ProcessIncoming()
{
	assert(mOpen);

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
					// Messages addressed to the NaviCtrl
					switch (msg.CmdID) {
					case RXCMD_DEBUG_HEADER:
						HandleDebugHeader(msg);
						break;
					case RXCMD_DEBUG_OUTPUT:
						HandleDebugOutput(msg);
						break;
					default:
						cerr << "Unknown MikroKopter command received: " << msg.CmdID << endl;
						break;
					}
				} else {
					cerr << "Unknown MikroKopter command received: " << msg.CmdID << endl;
				}
			}
		}

	} while (readBytes == RX_BUFFER_SIZE);
}

void MKConnection::SendBuffer(const Buffer_t& txBuffer)
{
	uint16_t length = txBuffer.DataBytes;
	uint8_t* data = txBuffer.pData;

	while (length > 0) {
		int sent = SendBuf(mComPortId, data, length);
		length -= sent;
		data += sent;
	}
}

void MKConnection::HandleDebugOutput(const SerialMsg_t& msg)
{
	const DebugOut_t *pDebugData = reinterpret_cast<const DebugOut_t*>(msg.pData);
	mDebugOutputCallback(*pDebugData);
}

void MKConnection::HandleDebugHeader(const SerialMsg_t& msg)
{
	const char *pDebugHeader = reinterpret_cast<const char*>(msg.pData);
	mDebugHeaderCallback(pDebugHeader);
}
