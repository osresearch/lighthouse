/** \file
 * XYZ position using the Vive Lighthouse beacons.
 */

#define IR1 5

void setup()
{
	pinMode(IR1, INPUT_PULLUP);
	Serial.begin(115200);
}


static inline boolean read_pin()
{
	//return bit_is_set(PINF,1);
	return digitalReadFast(IR1);
}

#define NUM_PULSES 64
uint32_t pulses[NUM_PULSES];
unsigned iter = 0;

void loop()
{
	// collect the timings on the next set of short pulses
	uint32_t sync_start = 0;

	for(int i = 0 ; i < NUM_PULSES ; )
	{
		while(!read_pin()) // PORTF & (1 << 1))
			;

		uint32_t pulse_start = micros();

		while(read_pin()) // !(PORTF & (1 << 1)))
			;

		uint32_t pulse_end = micros();
		uint32_t delta = pulse_end - pulse_start;
		if (delta > 40)
		{
			// sync pulse
			sync_start = pulse_start;
		} else {
			// sweep pulse
			pulses[i++] = pulse_start - sync_start;
		}
	}

	//Serial.print(iter++);

	for(int i = 0 ; i < NUM_PULSES ; i++)
	{
#if 0
		char s[32];
		sprintf(s, "%5d,%4d",
			(int) (pulses[i] - start),
			(int) (pulses[i+1] - pulses[i])
		);
		//Serial.print("  ");
		//Serial.print(pulses[i] - start);
		//Serial.print(' ');
		//Serial.print(pulses[i+1] - pulses[i]);
		Serial.println(s);
		start = pulses[i];
#endif
		Serial.println(pulses[i]);
	}

	Serial.println();
}
