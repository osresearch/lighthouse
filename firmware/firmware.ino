/** \file
 * XYZ position using the Vive Lighthouse beacons.
 *
 * This uses the BPW34 850nm photodiode and an amplifier circuit.
 *
 * The pulses are captured with the Teensy 3's "flexible timer"
 * that uses the 48 MHz system clock to record transitions on the
 * input lines.
 */

#include "InputCapture.h"

#define IR0 5
#define IR1 6
#define ICP_COUNT 2
InputCapture icp[ICP_COUNT];
uint32_t prev[ICP_COUNT];

void setup()
{
	pinMode(IR0, INPUT_PULLUP);
	pinMode(IR1, INPUT_PULLUP);
	icp[0].begin(IR0);
	icp[1].begin(IR1);

	Serial.begin(115200);
}


void loop()
{

	for(int i = 0 ; i < ICP_COUNT ; i++)
	{
		uint32_t val;
		int rc = icp[i].read(&val);
		if (rc == 0)
			continue;

		Serial.print(i),
		Serial.print(',');
		Serial.print(val);
		Serial.print(',');
		Serial.print(val - prev[i]);
		Serial.print(',');
		Serial.print(rc == -1 ? '0' : '1');
		Serial.println();

		prev[i] = val;
	}
}
