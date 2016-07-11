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
 *
 * The sync pulses are at 120 Hz, or roughly 8000 usec.  This means that
 * any pulse *longer* than 8 usec can be discarded since it is not
 * a valid measurement.
 *
 * The sync pulses are sent as one long pulse and one short one.
 * Since we are only capturing one edge, we see just the short pulse,
 * of around 400 usec.  If we can't see the lighthouse that this
 * time slot goes with, we'll see the next sync pulse at 8 usec later.
 * If we do see this lighthouse, we'll see a sweep pulse at time T,
 * then roughly 8 usec - T later the next sync.
 *
 */

#include "InputCapture.h"

#define IR0 5
#define IR1 6
#define IR2 9
#define IR3 10
#define ICP_COUNT 4
InputCapture icp[ICP_COUNT];
uint32_t prev[ICP_COUNT];
boolean saw_sync[ICP_COUNT];

void setup()
{
	icp[0].begin(IR0);
	icp[1].begin(IR1);
	icp[2].begin(IR2);
	icp[3].begin(IR3);

	Serial.begin(115200);
}


#if defined(KINETISK)
#define CLOCKS_PER_MICROSECOND ((double)F_BUS / 1000000.0)
#elif defined(KINETISL)
// PLL is 48 Mhz, which is 24 clocks per microsecond, but
// there is a divide by two for some reason.
#define CLOCKS_PER_MICROSECOND (F_PLL / 2000000)
#endif


void loop()
{
	for(int i = 0 ; i < ICP_COUNT ; i++)
	{
		uint32_t val;
		int rc = icp[i].read(&val);
		if (rc == 0)
			continue;

		const uint32_t delta = val - prev[i];
		prev[i] = val;
		char type = '?';

		if (rc == -1)
		{
			// we lost data.  reset the state
			type = 'L';
			saw_sync[i] = 0;
		} else
		if (delta < 500 * CLOCKS_PER_MICROSECOND)
		{
			// likely a sync pulse, the next one is a sweep
			type = 'X';
			saw_sync[i] = val;
		} else
		if (delta > 10000 * CLOCKS_PER_MICROSECOND)
		{
			// way too long, can't be a real one
			type = 'J';
			saw_sync[i] = 0;
		} else
		if (delta > 7500 * CLOCKS_PER_MICROSECOND)
		{
			// start of another sync pulse, but we didn't
			// get a sweep.
			type = 'B';
			saw_sync[i] = 0;
		} else
		if (saw_sync[i])
		{
			// we just saw a sync pulse and we aren't out-of-range,
			// so this is a sweep.
			type = 'S';
			saw_sync[i] = 0;
		} else {
			// A reasonable pulse, but we haven't seen the sync
			// so this is likely the post-sweep sync pulse start
			type = 'P';
			saw_sync[i] = 0;
		}

		Serial.print(val);
		Serial.print(',');
		Serial.print(i),
		Serial.print(',');
		Serial.print(type),
		Serial.print(',');
		Serial.print(delta);
		Serial.println();
	}
}
