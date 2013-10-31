#include "MK_DataLogger.h"

using namespace std;

static void sigterm_handler(int sig)
{
	received_sigterm = sig;
	received_nb_signals++;
	if (received_nb_signals > 3) exit(123);
}

void LogDebugHeader(bool lightDB)
{
	mLogFile << "Time,";
	uint8_t k = lightDB? 4 : 32;
	for (uint8_t i = 0; i < k; i++) {
		for (uint8_t j = 0; j < 16; j++) {
			mLogFile << mDebugLabels[i][j];
		}
		if (i < k - 1) mLogFile << ",";
	}
	mLogFile << endl;
}

void RecvDebugHeader(const char* header)
{
	// cout << "Received header label " << (int)header[0] << endl;
	for (int i = 0; i < 16; i++) {
		mDebugLabels[(int)header[0]][i] = header[i+1];
	}
	mDebugLabelCount |= 0x00000001 << header[0];
}

void RecvDebugOutput(const DebugOut_t& data)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	// cout << "Received debug data" << endl;
	mLogFile << (float)((now.tv_sec - StartTime.tv_sec) * 1000000L
			+ (now.tv_usec - StartTime.tv_usec)) / 1.0e6 << ",";
	for (uint8_t i = 0; i < 32; ++i) {
		mLogFile << data.Analog[i];
		if (i < 31) mLogFile << ",";
	}
	mLogFile << endl;
}

void RecvDebugSmOut(const DebugSmOut_t& data)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	// cout << "Received debug data" << endl;
	mLogFile << (float)((now.tv_sec - StartTime.tv_sec) * 1000000L
			+ (now.tv_usec - StartTime.tv_usec)) / 1.0e6 << ",";
	for (uint8_t i = 0; i < 4; ++i) {
		mLogFile << data.Analog[i];
		if (i < 3) mLogFile << ",";
	}
	mLogFile << endl;
}

bool ParseOptions(int argc, char *argv[], int *port, int *frequency, string *filename,
		bool *getHeader, bool *lightDB, bool *motorTest)
{
	// Possible options:
	// -f output file name
	// -h show usage
	// -l light debug
	// -n no header
	// -p communication port
	// -r capture rate (Hz)
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
			} else if (argv[i][1] == 'l') {
				*lightDB = true;
			} else if (argv[i][1] == 'm') {
				*motorTest = true;
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
					cerr << "-p option requires a frequency in Hz (2 - 250)" << endl;
					return true;
				} else {
					*frequency = atoi(argv[i+1]);
					i++;
				}
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
	bool getHeader = true, lightDB = false, motorTest = false;
	int port = 16, frequency = 10;
	string filename = "mk_datalog.csv";

	signal(SIGQUIT, sigterm_handler); /* Quit (POSIX).  */
	signal(SIGINT , sigterm_handler); /* Interrupt (ANSI).  */
	signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

	cout << endl;
	cout << "  MK_DataLogger" << endl;
	cout << "  -------------" << endl;
	cout << endl;

	if (ParseOptions(argc, argv, &port, &frequency, &filename, &getHeader, &lightDB, &motorTest)) {
		cout << "usage: " << argv[0] << " [-options ...]" << endl;
		cout << "options:" << endl;
		cout << "    -f <filename>   Specify outout file name" << endl;
		cout << "    -h              Print this usage message" << endl;
		cout << "    -l              Switch to \"light debug\" (up to 250 Hz)" << endl;
		cout << "    -m              Run motor test" << endl;
		cout << "    -n              No header in the output file" << endl;
		cout << "    -p <port>       Specify serial commport" << endl;
		cout << "    -r <rate>       Specify logging rate in Hz (2 - 55)" << endl;
		cout << endl;
		return 1;
	}
	frequency = min(max(frequency, lightDB? 4 : 2), (lightDB? 250 : 55));

	// TODO: make filename a paramter
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

	if (motorTest) {
		cout << "  WARNING: You have requested to activte a motor!" << endl;
		cout << "  Type \"Y\" and hit \"Enter\" to continue." << endl;
		cout << endl;
		cout << "> ";
		fflush(stdin);
		char c = getchar();
		if (c != 'Y') return 0;
	}

	const int LOOP_FREQUENCY = max(frequency * 2, 32); // Run loop twice as fast as expected data rate
	const int HEADER_RQST_TIMER = LOOP_FREQUENCY / 32; // Attempt to receive header labels in 1 second
	const int OUTPUT_RQST_TIMER = LOOP_FREQUENCY / 4; // Send a fresh request every 2 seconds
	const int MOTOR_TEST_TIMER = LOOP_FREQUENCY / 4; // Send the motor test request 4 times per second
	const uint8_t OUTPUT_PERIOD = (lightDB? 1000 : 100) / frequency; // Debug output period request in ms

	const timespec REQ = {0, 1000000000 / LOOP_FREQUENCY};
	timespec rem; // remainder of interrupted nanosleep (unused);

	SetDebugHeaderCallback(bind(RecvDebugHeader, placeholders::_1));
	SetDebugOutputCallback(bind(RecvDebugOutput, placeholders::_1));
	SetDebugSmOutCallback(bind(RecvDebugSmOut, placeholders::_1));

	if (getHeader) cout << "Receiving header text" << endl;
	else cout << "Starting debug logging" << endl;

	while (received_sigterm == 0) {
		static uint16_t counter = 0xFFFF, counter2 = 0;

		ProcessIncoming();

		// Request debug data being sent from the MK, this has to be done every few seconds or
		// the MK will stop sending the data
		if (getHeader && (counter > HEADER_RQST_TIMER)) {
			if ((mDebugLabelCount != 0xFFFFFFFF)
					&& (!lightDB || (mDebugLabelCount != 0x0000000F))) {
				SendDebugHeaderRequest(lightDB);
				counter = 0;
			} else {
				cout << "Finished receving header" << endl;
				cout << "Starting debug logging" << endl;
				LogDebugHeader(lightDB);
				getHeader = false;
				counter = OUTPUT_RQST_TIMER;
				gettimeofday(&StartTime, NULL);
			}
		} else if (!getHeader && (counter > OUTPUT_RQST_TIMER)) {
			SendDebugOutputRequest(OUTPUT_PERIOD, lightDB);
			counter = 0;
		}
		else if (motorTest && !getHeader) {
			static int16_t motorSetPoint = 15;
			static int16_t step = 30;
			if (!(counter2 % MOTOR_TEST_TIMER)) {
				SendMotorTestRequest((uint8_t)motorSetPoint);

				if (counter2 == 10 * MOTOR_TEST_TIMER) {
					if (motorSetPoint <= 15 && step < 0) {
						if (step > -240) step *= -2;
						else motorTest = false;
					}
					else if (motorSetPoint + step > 255) step = -step;
					motorSetPoint += step;
					counter2 = 0;
					cout << "Reguesting motor test value of " << motorSetPoint << endl;
				}
			}
			counter2++;
		}
		counter++;

		nanosleep(&REQ, &rem);  // loop twice per expected reception interval
	}

	if(motorTest) SendMotorTestRequest(0);

	cout << endl;
	cout << "Received termination signal" << endl;
	cout << "Closing log file" << endl;
	mLogFile.close();

	return 0;
}
