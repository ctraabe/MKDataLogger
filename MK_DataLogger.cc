#include "MK_DataLogger.h"

using namespace std;

static void sigterm_handler(int sig)
{
	received_sigterm = sig;
	received_nb_signals++;
	if (received_nb_signals > 3) exit(123);
}

void LogControlValues()
{
	for (size_t i = 0; i < 32; ++i) {
		mLogFile << mMKDebugOutput.Analog[i];
		if (i < 31) mLogFile << ",";
	}

	mLogFile << endl;
}

void RecvDebugHeader(const char* header)
{
	mMKDebugHeader = *header;
}

void RecvDebugOutput(const DebugOut_t& data)
{
	mMKDebugOutput = data;
	LogControlValues();
}

uint8_t ConnectToMK(int nComPortId, int nComBaudrate)
{
	using namespace std::placeholders;

	// Attempt connecting to the MK NaviCtrl
	mMKConn = MKConnection(nComPortId, nComBaudrate);
	if (!mMKConn) {
		cerr << "Failed to connect to MikroKopter." << endl;
		return 1;
	} else {
		// Hook up all the callback functions
		mMKConn.SetDebugHeaderCallback(bind(RecvDebugHeader, _1));
		mMKConn.SetDebugOutputCallback(bind(RecvDebugOutput, _1));
		return 0;
	}
}

void ParseOptions(int argc, char *argv[])
{
}

int main(int argc, char *argv[])
{
	// TODO: make this macro a parameter
#define INTERVAL 100

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
	if (ConnectToMK(16, 57600)) return 1;

	while (received_sigterm == 0) {
		static uint16_t counter = 0;

		if (mMKConn) {
			mMKConn.ProcessIncoming();

			// Request debug data being sent from the MK, this has to be done every few seconds or
			// the MK will stop sending the data
			if (counter > 4000 / INTERVAL) { // 2 seconds
				mMKConn.SendDebugOutputInterval(INTERVAL);
				counter = 0;
			}
			else {
				counter++;
			}
		}
		usleep(INTERVAL * 500);  // loop twice per expected reception interval
	}

	return 0;
}
