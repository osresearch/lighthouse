/** \file
 * XYZ position using the Vive Lighthouse beacons.
 *
 * This uses the BPW34 850nm photodiode and an amplifier circuit.
 *
 * The pulses are captured with the Teensy 3's "flexible timer"
 * that uses the 48 MHz system clock to record transitions on the
 * input lines.
 *
 * With the opamp circuit, I found it best to use a 100k feedback on 
 * the first stage and to not include the 220K feedback resistor on
 * the second stage.
 */

#include "InputCapture.h"

#define IR0 9
#define IR1 10
#define IR2 22
#define IR3 23
#define ICP_COUNT 4
InputCapture icp[ICP_COUNT];
uint32_t prev[ICP_COUNT];

void setup()
{
	pinMode(IR0, INPUT);
	pinMode(IR1, INPUT);
	pinMode(IR2, INPUT);
	pinMode(IR3, INPUT);

	icp[0].begin(IR0);
	icp[1].begin(IR1);
	icp[2].begin(IR2);
	icp[3].begin(IR3);

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

#if defined(KINETISK)
#define CLOCKS_PER_MICROSECOND ((double)F_BUS / 1000000.0)
#elif defined(KINETISL)
#define CLOCKS_PER_MICROSECOND ((double)F_PLL / 2000000.0)
#endif
		//float delta = (val - prev[i]) / CLOCKS_PER_MICROSECOND;
		//Serial.print(delta);

		Serial.print(val - prev[i]);
		Serial.print(',');
		Serial.print(rc == -1 ? '0' : '1');
		Serial.println();

		prev[i] = val;
	}
}
