/** \file
 * XYZ position using the Vive Lighthouse beacons.
 *
 * This uses up to four Triad Semiconductor TS3633-CM1 sensor modules,
 * which have a built-in 850nm photodiode and an amplifier circuit.
 *
 * The pulses are captured with the Teensy 3's "flexible timer"
 * that uses the 48 MHz system clock to record transitions on the
 * input lines.
 *
 * The sync pulses are at 120 Hz, or roughly 8000 usec.  This means that
 * any pulse *longer* than 8 usec can be discarded since it is not
 * a valid measurement.
 *
 * The Teensy FTM might be able to capture both rising and falling edges,
 * but we can fake it by using all eight input channels for four
 * sensors.
 *
 * If we can't see the lighthouse that this
 * time slot goes with, we'll see the next sync pulse at 8 usec later.
 * If we do see this lighthouse, we'll see a sweep pulse at time T,
 * then roughly 8 usec - T later the next sync.
 *
 * Meaning of the sync pulses lengths:
 * https://github.com/nairol/LighthouseRedox/blob/master/docs/Light%20Emissions.md
 */

#include "InputCapture.h"

#define IR0 5
#define IR1 6
#define IR2 9
#define IR3 10
#define IR4 20
#define IR5 21
#define IR6 22
#define IR7 23
#define ICP_COUNT 8
InputCapture icp[ICP_COUNT];
uint32_t prev[ICP_COUNT];
boolean saw_sync[ICP_COUNT];

void setup()
{
	icp[0].begin(IR0);
	icp[1].begin(IR1, RISING);
	icp[2].begin(IR2);
	icp[3].begin(IR3, RISING);
	icp[4].begin(IR4);
	icp[5].begin(IR5, RISING);
	icp[6].begin(IR6);
	icp[7].begin(IR7, RISING);

	Serial.begin(115200);
}


#if defined(KINETISK)
//#define CLOCKS_PER_MICROSECOND ((double)F_BUS / 1000000.0)
#define CLOCKS_PER_MICROSECOND (F_BUS / 1000000)
#elif defined(KINETISL)
// PLL is 48 Mhz, which is 24 clocks per microsecond, but
// there is a divide by two for some reason.
#define CLOCKS_PER_MICROSECOND (F_PLL / 2000000)
#endif


uint8_t rotor;

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

		// if this was a falling edge, wait for the rising edge
		if ((i & 1) == 0)
			continue;

		const uint32_t len = val - prev[i-1];

		if (len < 20 * CLOCKS_PER_MICROSECOND)
		{
			// Sweep!
			// todo: use the midpoint of the pulse
			// todo: filter if we don't know the axis
			int valid = delta < 8000 * CLOCKS_PER_MICROSECOND;
			
			Serial.print(i/2);
			Serial.print(",S,");
			Serial.print(delta / CLOCKS_PER_MICROSECOND);
			Serial.print(",");
			Serial.print(valid);
			Serial.print(",");
			Serial.print(len);
			Serial.println();
			continue;
		}

		int skip = 9;
		int rotor = 9;
		int data = 9;
		const char * name = "??";

		const unsigned window = 4 * CLOCKS_PER_MICROSECOND;

		static const unsigned midpoints[] = {
			(unsigned) (62.5 * CLOCKS_PER_MICROSECOND),
			(unsigned) (83.3 * CLOCKS_PER_MICROSECOND),
			(unsigned) (72.9 * CLOCKS_PER_MICROSECOND),
			(unsigned) (93.8 * CLOCKS_PER_MICROSECOND),
			(unsigned) (104.0 * CLOCKS_PER_MICROSECOND),
			(unsigned) (125.0 * CLOCKS_PER_MICROSECOND),
			(unsigned) (115.0 * CLOCKS_PER_MICROSECOND),
			(unsigned) (135.0 * CLOCKS_PER_MICROSECOND),
		};

		static const char * const names[] = {
			"j0", "j1", "k0", "k1", "j2", "j3", "k2", "k3",
		};

		for (int i = 0 ; i < 8 ; i++)
		{
			if (len < midpoints[i] - window)
				continue;
			if (len > midpoints[i] + window)
				continue;

			skip = (i >> 2) & 1;
			rotor = (i >> 1) & 1;
			data = (i >> 0) & 1;
			name = names[i];
			break;
		}

		Serial.print(i);
		Serial.print(",X,");
		Serial.print(name);
		Serial.print(",");
		Serial.print(skip);
		Serial.print(",");
		Serial.print(rotor);
		Serial.print(",");
		Serial.print(data);
		Serial.print(",");
		Serial.print(len / CLOCKS_PER_MICROSECOND);
		Serial.println();
		
#if 0
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
		Serial.print(delta / CLOCKS_PER_MICROSECOND);
		Serial.print(',');

		const unsigned long delta2 = val - prev[i & 1 ? i-1 : i+1];
		Serial.print(delta2 / CLOCKS_PER_MICROSECOND);
		Serial.println();
#endif
	}
}
