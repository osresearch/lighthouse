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

#include "LighthouseSensor.h"
#include "LighthouseXYZ.h"


#define IR0 5
#define IR1 6
#define IR2 9
#define IR3 10
#define IR4 20
#define IR5 21
#define IR6 22
#define IR7 23


// Lighthouse sources rotation matrix & 3d-position
// needs to be computed and read from EEPROM instead of constant.
static lightsource lightsources[2] = {{
    {  -0.88720f,  0.25875f, -0.38201f,
       -0.04485f,  0.77566f,  0.62956f,
        0.45920f,  0.57568f, -0.67656f},
    {  -1.28658f,  2.32719f, -2.04823f}
}, {
    {   0.52584f, -0.64026f,  0.55996f,
        0.01984f,  0.66739f,  0.74445f,
       -0.85035f, -0.38035f,  0.36364f},
    {   1.69860f,  2.62725f,  0.92969f}
}};


LighthouseSensor sensors[4];
LighthouseXYZ xyz[4];

void setup()
{
	sensors[0].begin(0, IR0, IR1);
	sensors[1].begin(1, IR2, IR3);
	sensors[2].begin(2, IR4, IR5);
	sensors[3].begin(3, IR6, IR7);

	for(int i = 0 ; i < 4 ; i++)
		xyz[i].begin(i, &lightsources[0], &lightsources[1]);

	Serial.begin(115200);
}

static char hexdigit(unsigned val)
{
	val &= 0XF;
	if (val <= 9)
		return '0' + val;
	else
		return 'A' + val - 0xA;
}

static void print_ootx(LighthouseOOTX & o)
{
	Serial.print(o.length);
	for(unsigned i = 0 ; i < o.length ; i++)
	{
		const uint8_t b = o.bytes[i];
		Serial.print(" ");
		Serial.print(hexdigit(b >> 4));
		Serial.print(hexdigit(b >> 0));
	}

	Serial.println();

	// flag that we have processed this message
	o.complete = 0;
}


void loop()
{
	for(int i = 0 ; i < 4 ; i++)
	{
		LighthouseSensor * const s = &sensors[i];
		LighthouseXYZ * const p = &xyz[i];

		int ind = s->poll();
		if (ind < 0)
			continue;

		if (s->ootx.complete)
			print_ootx(s->ootx);

		if (!p->update(ind, s->angles[ind]))
			continue;

		Serial.print(i);
		Serial.print(",");
		Serial.print(s->raw[0]);
		Serial.print(",");
		Serial.print(s->raw[1]);
		Serial.print(",");
		Serial.print(s->raw[2]);
		Serial.print(",");
		Serial.print(s->raw[3]);
		Serial.print(",");
		Serial.print((int)(p->xyz[0]*1000));
		Serial.print(",");
		Serial.print((int)(p->xyz[1]*1000));
		Serial.print(",");
		Serial.print((int)(p->xyz[2]*1000));
		Serial.print(",");
		Serial.println(p->dist);
	}
}
