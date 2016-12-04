/** \file
 * Geometry for computing the position of a sensor with the Lighthouses.
 */
#ifndef _lighthouse_h_
#define _lighthouse_h_

#include <arm_math.h>

void
lighthouse_compute(
	const float angles[], // from the first lighthouse
	float (*ned)[3], // output of computed position
	float * dist
);

#endif
