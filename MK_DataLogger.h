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

#include <sys/time.h>

// MK message handlers
void RecvDebugHeader(const char* header);
void RecvDebugOutput(const DebugOut_t& data);
void RecvDebugSmOut(const DebugSmOut_t& data);

void LogDebugHeader();

static volatile int received_sigterm = 0;
static volatile int received_nb_signals = 0;

uint32_t mDebugLabelCount = 0x00000000;
char mDebugLabels[32][16];
struct timeval StartTime;
std::ofstream mLogFile;

#endif
