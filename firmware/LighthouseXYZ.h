/** \file
 * Geometry for computing the position of a sensor with the Lighthouses.
 *
 * This computes the XYZ position of a single sensor, based on the
 * four laser angle measurements from the two lighthouses and the
 * computed position/angles of the lighthouses.
 */
#ifndef _lighthouse_h_
#define _lighthouse_h_

static const int vec3d_size = 3;
typedef float vec3d[vec3d_size];
typedef float mat33[3*3];

struct lightsource {
    mat33 mat;
    vec3d origin;
};


class LighthouseXYZ
{
public:
	LighthouseXYZ() {};

	void begin(int id, lightsource * lh1, lightsource * lh2);

	bool update(unsigned ind, float angle);

	vec3d xyz;
	float dist;

private:
	int id;
	lightsource * lighthouse[2];

	unsigned fresh;
	float angles[4];

	bool compute();
};


#endif
