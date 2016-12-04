/** \file
 * Geometry for computing the position of a sensor with the Lighthouses.
 *
 * This computes the XYZ position of a single sensor, based on the
 * four laser angle measurements from the two lighthouses and the
 * computed position/angles of the lighthouses.
 */
#ifndef _lighthouse_h_
#define _lighthouse_h_

struct lightsource {
    float mat[9];
    float origin[3];
};


class LighthouseXYZ
{
public:
	LighthouseXYZ() {};

	void begin(int id, lightsource * lh1, lightsource * lh2);

	bool update(unsigned ind, float angle);

	float xyz[3];
	float dist;

private:
	int id;
	lightsource * lighthouse[2];

	unsigned fresh;
	float angles[4];

	bool compute();
};


#endif
