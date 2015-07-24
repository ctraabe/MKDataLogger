#ifndef __MAIN_H
#define __MAIN_H

#include "mk_connection.hpp"

#include <csignal>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <sys/time.h>

void LogDebugHeader();
void LogDebugOutput();

// MK message handlers
void RecvDebugHeader(const char* header);
void RecvDebugOutput(const DebugOut_t& data);

static volatile int received_sigterm = 0;
static volatile int received_nb_signals = 0;

uint32_t mLabelsRcvd = 0x00000000;
char mLabels[32][16];
DebugOut_t mMKDebugOutput;
HighSpeed_t mMKHighSpeedOutput;
std::ofstream mLogFile;

#endif  // __MAIN_H
