#include "MK_DataLogger.h"

using namespace std;

static void sigterm_handler(int sig)
{
	received_sigterm = sig;
	received_nb_signals++;
	if (received_nb_signals > 3) exit(123);
}

void LogHeader(bool highSpeed)
{
	int n = highSpeed ? 10 : 32;
	for (uint8_t i = 0; i < n; i++) {
		for (uint8_t j = 0; j < 16; j++) {
			mLogFile << mLabels[i][j];
		}
		if (i < n - 1) mLogFile << ",";
	}
	mLogFile << endl;
}

void LogOutput(bool highSpeed)
{
	// struct timeval now;
	// gettimeofday(&now, NULL);
	// mLogFile << (float)((now.tv_sec - StartTime.tv_sec) * 1000000L
	// 		+ (now.tv_usec - StartTime.tv_usec)) / 1.0e6 << ",";

	if (highSpeed) {
		for (uint8_t i = 0; i < 8; ++i) {
			mLogFile << mMKHighSpeedOutput.int16[i] << ",";
		}
		mLogFile << (int)mMKHighSpeedOutput.uint8[0] << ",";
		mLogFile << (int)mMKHighSpeedOutput.uint8[1];
	} else {
		for (uint8_t i = 0; i < 32; ++i) {
			mLogFile << mMKDebugOutput.Analog[i];
			if (i < 31) mLogFile << ",";
		}
	}
	mLogFile << endl;
}

void RecvHeader(const char* header)
{
	int labelNumber = (int)header[0];
	if (labelNumber > 32) labelNumber -= 32;
	// cout << "Received header label " << labelNumber << endl;
	for (int i = 0; i < 16; i++) {
		mLabels[labelNumber][i] = header[i+1];
	}
	mLabelsRcvd |= 0x00000001 << labelNumber;
}

void RecvDebugOutput(const DebugOut_t& data)
{
	// cout << "Received debug data" << endl;
	mMKDebugOutput = data;
	LogOutput(false);
}

void RecvHighSpeedOutput(const HighSpeed_t& data)
{
	// cout << "Received high-speed data" << endl;
	mMKHighSpeedOutput = data;
	LogOutput(true);
}

bool ParseOptions(int argc, char *argv[], int *port, int *frequency,
		string *filename, bool *getHeader, bool *highSpeed)
{
	// Possible options:
	// -f output file name
	// -h show usage
	// -n no header
	// -p communication port
	// -r capture rate (Hz)
	// -i High-speed data
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'f') {
				if ((i == argc - 1) || argv[i+1][0] == '-') {
					cerr << "-f option requires a filename" << endl;
					return true;
				} else {
					*filename = argv[i+1];
					i++;
				}
			} else if (argv[i][1] == 'h') {
				return true;
			} else if (argv[i][1] == 'n') {
				*getHeader = false;
			} else if (argv[i][1] == 'p') {
				if ((i == argc - 1) || argv[i+1][0] == '-') {
					cerr << "-p option requires a port number" << endl;
					return true;
				} else {
					*port = atoi(argv[i+1]);
					i++;
				}
			} else if (argv[i][1] == 'r') {
				if ((i == argc - 1) || argv[i+1][0] == '-') {
					cerr << "-p option requires a frequency in Hz (2 - 100)" << endl;
					return true;
				} else {
					*frequency = atoi(argv[i+1]);
					i++;
				}
			} else if (argv[i][1] == 'i') {
				*highSpeed = true;
			}
		} else {
			cerr << "Unrecognized argument" << endl;
			return true;
		}
	}
	return false;
}

int main(int argc, char *argv[])
{
	bool getHeader = true, highSpeed = true;
	int port = 16, frequency = 10;
	string filename = "sensor_data.csv";

	signal(SIGQUIT, sigterm_handler); /* Quit (POSIX).  */
	signal(SIGINT , sigterm_handler); /* Interrupt (ANSI).  */
	signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

	cout << endl;
	cout << "  MK_DataLogger" << endl;
	cout << "  -------------" << endl;
	cout << endl;

	if (ParseOptions(argc, argv, &port, &frequency, &filename, &getHeader,
			&highSpeed)) {
		cout << "usage: " << argv[0] << " [-options ...]" << endl;
		cout << "options:" << endl;
		cout << "    -f <filename>   Specify output file name" << endl;
		cout << "    -h              Print this usage message" << endl;
		cout << "    -n              No header in the output file" << endl;
		cout << "    -p <port>       Specify serial commport" << endl;
		cout << "    -r <rate>       Specify logging rate in Hz (2 - 100)" << endl;
		cout << "    -i              High-speed mode (custom firmware)" << endl;
		cout << endl;
		return 1;
	}

	if (highSpeed)
		frequency = 128;
	else if (frequency < 2)
		frequency = 2;
	else if (frequency > 100)
		frequency = 100;

	mLogFile.open(filename, ios::out | ios::trunc);
	if (!mLogFile) {
		cerr << "Failed to open " << filename << endl;
		return 1;
	}

	if (OpenMKConnection(port, 57600)) {
		cerr << "Could not connect on port " << port << endl;
		return 1;
	}

	cout << "  Communicating on port " << port << endl;
	cout << "  Requesting debug data at " << frequency << " Hz" << endl;
	cout << "  Writing to file " << filename << endl;
	cout << endl;

	const int LOOP_FREQUENCY = frequency * 2; // Run loop twice as fast as expected data rate
	const int HEADER_RQST_TIMER = (LOOP_FREQUENCY >> 5) + 1; // Attempt to receive header labels in 1 second
	const int OUTPUT_RQST_TIMER = 2 * LOOP_FREQUENCY; // Send a fresh request every 2 seconds
	const int OUTPUT_PERIOD = 100 / frequency; // Debug output period request in hundredths of a second

	const timespec REQ = {0, 1000000000 / LOOP_FREQUENCY};
	timespec rem; // remainder of interrupted nanosleep (unused);

	SetHeaderCallback(bind(RecvHeader, placeholders::_1));
	SetDebugOutputCallback(bind(RecvDebugOutput, placeholders::_1));
	SetHighSpeedOutputCallback(bind(RecvHighSpeedOutput, placeholders::_1));

	if (getHeader) cout << "Receiving header text" << endl;

	while (received_sigterm == 0) {
		static uint16_t counter = 0xFFFF;

		ProcessIncoming();

		// Request debug data being sent from the MK, this has to be done every few seconds or
		// the MK will stop sending the data
		if (getHeader && (counter > HEADER_RQST_TIMER)) {
			if ((highSpeed && (mLabelsRcvd != 0x000003FF))
					|| (!highSpeed && (mLabelsRcvd != 0xFFFFFFFF))) {
				SendHeaderRequest(highSpeed);
				counter = 0;
			} else {
				cout << "Finished receiving header" << endl;
				cout << "Starting debug logging" << endl;
				LogHeader(highSpeed);
				getHeader = false;
				counter = OUTPUT_RQST_TIMER  + 1;
			}
		} else if (!getHeader && (counter > OUTPUT_RQST_TIMER)) {
			SendOutputRequest(OUTPUT_PERIOD);
			counter = 0;
		}
		if (getHeader || !highSpeed) counter++;

		nanosleep(&REQ, &rem);  // loop twice per expected reception interval
	}

	cout << endl;
	cout << "Received termination signal" << endl;
	cout << "Closing log file" << endl;
	mLogFile.close();
	// Send a bunch of cancel requests
	if (highSpeed) {
		for (int i = 16; i; i--) {
			nanosleep(&REQ, &rem);  // loop twice per expected reception interval
			nanosleep(&REQ, &rem);  // loop twice per expected reception interval
			nanosleep(&REQ, &rem);  // loop twice per expected reception interval
			nanosleep(&REQ, &rem);  // loop twice per expected reception interval
			nanosleep(&REQ, &rem);  // loop twice per expected reception interval
			nanosleep(&REQ, &rem);  // loop twice per expected reception interval
			nanosleep(&REQ, &rem);  // loop twice per expected reception interval
			nanosleep(&REQ, &rem);  // loop twice per expected reception interval
			SendHighSpeedResetRequest();
		}
	}

	return 0;
}
