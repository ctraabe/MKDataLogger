#ifndef __MK_DATALOGGER_H
#define __MK_DATALOGGER_H

#include "MK_Connection.h"

#include <csignal>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>

	uint8_t ConnectToMK(int nComPortId, int nComBaudrate);
	void LogControlValues();

	// MK message handlers
	void RecvDebugHeader(const char* header);
	void RecvDebugOutput(const DebugOut_t& data);

	static volatile int received_sigterm = 0;
	static volatile int received_nb_signals = 0;

	static MKConnection mMKConn;
	char mMKDebugHeader;
	DebugOut_t mMKDebugOutput;
	std::ofstream mLogFile;

#endif
