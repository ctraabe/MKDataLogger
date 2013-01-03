#include "MK_DataLogger.h"

using namespace std;

static void sigterm_handler(int sig)
{
	received_sigterm = sig;
	received_nb_signals++;
	if (received_nb_signals > 3) exit(123);
}

void LogDebugHeader()
{
	for (uint8_t i = 0; i < 32; i++) {
		for (uint8_t j = 0; j < 16; j++) {
			mLogFile << mDebugLabels[i][j];
		}
		if (i < 31) mLogFile << ",";
	}
	mLogFile << endl;
}

void LogDebougOutput()
{
	for (uint8_t i = 0; i < 32; ++i) {
		mLogFile << mMKDebugOutput.Analog[i];
		if (i < 31) mLogFile << ",";
	}
	mLogFile << endl;
}

void RecvDebugHeader(const char* header)
{
	cout << "Received header label " << (int)header[0] << endl;
	for (int i = 0; i < 16; i++) {
		mDebugLabels[(int)header[0]][i] = header[i+1];
	}
	mDebugLabelCount |= 0x00000001 << header[0];
}

void RecvDebugOutput(const DebugOut_t& data)
{
	cout << "Received debug data" << endl;
	mMKDebugOutput = data;
	LogDebougOutput();
}

void ParseOptions(int argc, char *argv[])
{
}

int main(int argc, char *argv[])
{
	// TODO: make this macro a parameter
	// Frequency can go up to 100 Hz (limited by FlightCtrl)
	enum { FREQUENCY = 10 }; // Run loop twice as fast as expected data rate

	enum { LOOP_FREQUENCY = FREQUENCY * 2 }; // Run loop twice as fast as expected data rate
	enum { HEADER_RQST_TIMER = (LOOP_FREQUENCY >> 5) + 1 }; // Attempt to receive header labels in 1 second
	enum { OUTPUT_RQST_TIMER = 2 * LOOP_FREQUENCY }; // Send a fresh request every 2 seconds
	enum { OUTPUT_PERIOD = 100 / FREQUENCY }; // Debug output period request in hundredths of a second

	const timespec req = {0, 1000000000 / LOOP_FREQUENCY};
	timespec rem; // remainder of interrupted nanosleep (unused);

	signal(SIGQUIT, sigterm_handler); /* Quit (POSIX).  */
	signal(SIGINT , sigterm_handler); /* Interrupt (ANSI).  */
	signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

	cout << endl;
	cout << "  MK_DataLogger" << endl;
	cout << "  -------------" << endl;
	cout << endl;

	ParseOptions(argc, argv);

	// TODO: make filename a paramter
	mLogFile.open("mk_datalog.txt", ios::out | ios::trunc);
	if (!mLogFile) {
		cerr << "Failed to open mk_datalog.txt" << endl;
		return 1;
	}

	// TODO: parameterize these inputs
	if (OpenMKConnection(16, 57600)) return 1;

	SetDebugHeaderCallback(bind(RecvDebugHeader, placeholders::_1));
	SetDebugOutputCallback(bind(RecvDebugOutput, placeholders::_1));

	while (received_sigterm == 0) {
		static uint16_t counter = 0xFFFF;
		static bool getHeader = true;

		ProcessIncoming();

		// Request debug data being sent from the MK, this has to be done every few seconds or
		// the MK will stop sending the data
		if (getHeader && (counter > HEADER_RQST_TIMER)) {
			if (mDebugLabelCount != 0xFFFFFFFF) {
				SendDebugHeaderRequest();
				counter = 0;
			} else {
				LogDebugHeader();
				getHeader = false;
				counter = OUTPUT_RQST_TIMER;
			}
		} else if (!getHeader && (counter > OUTPUT_RQST_TIMER)) {
			SendDebugOutputRequest(OUTPUT_PERIOD);
			counter = 0;
		}
		counter++;

		nanosleep(&req, &rem);  // loop twice per expected reception interval
	}

	mLogFile.close();

	return 0;
}
