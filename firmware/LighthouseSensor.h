/** \file
 * Raw interface to the TS3633 sensor.
 *
 */
#ifndef _LighthouseSensor_h_
#define _LighthouseSensor_h_

#include "InputCapture.h"
#include "LighthouseOOTX.h"

class LighthouseSensor
{
public:
	LighthouseSensor() {}

	// Two input capture pins, one for rising, one for falling
	void begin(int id, int input_capture0, int input_capture1);

	// Check for any activity,
	// return the sample index if a new angle measurement is available
	int poll();

	// Measured angles from the sweep pulses
	uint32_t raw[4];
	float angles[4];

	static const bool debug = 0;

	LighthouseOOTX ootx;

private:
	int id;
	InputCapture icp_rising;
	InputCapture icp_falling;

	// process a sweep pulse and return -1 if no new pulse detected
	int sweep_pulse(unsigned when, unsigned len, unsigned duty);

	// What was the last pulse times in each direction?
	uint32_t last_rising;
	uint32_t last_falling;

	// When did the non-skipped rotor report 0 degrees?
	uint32_t zero_time;

	// Which rotor was reported by the sync pulse?
	unsigned axis;

	// Have we seen a sweep pulse?
	unsigned got_sweep;

	// Have we seen a sync pulse that says skip?
	unsigned got_skip;

	// Have we seen a sync pulse that says not-skipped?
	unsigned got_not_skip;

	// Which lighthouse did we compute this sweep was for?
	unsigned lighthouse;
};

#endif
