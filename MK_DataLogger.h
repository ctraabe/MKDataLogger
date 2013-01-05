#ifndef __MK_DATALOGGER_H
#define __MK_DATALOGGER_H

#include "MK_Connection.h"

#include <csignal>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>

void LogDebugHeader();
void LogDebugOutput();

// MK message handlers
void RecvDebugHeader(const char* header);
void RecvDebugOutput(const DebugOut_t& data);

static volatile int received_sigterm = 0;
static volatile int received_nb_signals = 0;

uint32_t mDebugLabelCount = 0x00000000;
char mDebugLabels[32][16];
DebugOut_t mMKDebugOutput;
std::ofstream mLogFile;

#endif
