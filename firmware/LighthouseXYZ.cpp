// adapted from https://github.com/ashtuchkin/vive-diy-position-sensor
#include "LighthouseXYZ.h"
#include <arm_math.h>

static const int vec3d_size = 3;
typedef float vec3d[vec3d_size];
typedef float mat33[3*3];

static void
vec_cross_product(
	const vec3d &a,
	const vec3d &b,
	vec3d &res
)
{
	res[0] = a[1]*b[2] - a[2]*b[1];
	res[1] = a[2]*b[0] - a[0]*b[2];
	res[2] = a[0]*b[1] - a[1]*b[0];
}


static float
vec_length(
	vec3d &vec
)
{
	float pow, res;

	arm_power_f32(vec, vec3d_size, &pow);
	arm_sqrt_f32(pow, &res);

	return res;
}

static void
calc_ray_vec(
	float rotation[9],
	float angle1,
	float angle2,
	vec3d &res
)
{
	// Normal vector to X plane
	vec3d a = {
		+arm_cos_f32(angle1),
		0,
		-arm_sin_f32(angle1),
	};

	// Normal vector to Y plane
	vec3d b = {
		0,
		+arm_cos_f32(angle2),
		+arm_sin_f32(angle2),
	};

	vec3d ray = {};

	// Intersection of two planes -> ray vector.
	vec_cross_product(b, a, ray);

	// Normalize ray length.
	float len = vec_length(ray);
	arm_scale_f32(ray, 1/len, ray, vec3d_size);

	arm_matrix_instance_f32 source_rotation_matrix = {3, 3, rotation};
	arm_matrix_instance_f32 ray_vec = {3, 1, ray};
	arm_matrix_instance_f32 ray_rotated_vec = {3, 1, res};

	// rotate the ray by the lighthouse's rotation matrix,
	// which translates the lighthouse-relative ray into
	// the XYZ-space.
	arm_mat_mult_f32(&source_rotation_matrix, &ray_vec, &ray_rotated_vec);
}




void LighthouseXYZ::begin(
	int id,
	lightsource * lh1,
	lightsource * lh2
)
{
#if 0
	// Convert Y up -> Z down; then rotate XY around Z clockwise and inverse X & Y
	this->ned_rotation[0] = -cosf(ne_angle);
	this->ned_rotation[1] =  0.0f;
	this->ned_rotation[2] =  sinf(ne_angle);

	this->ned_rotation[3] = -sinf(ne_angle);
	this->ned_rotation[4] =  0.0f;
	this->ned_rotation[5] = -cosf(ne_angle);

	this->ned_rotation[6] =  0.0f;
	this->ned_rotation[7] = -1.0f;
	this->ned_rotation[8] =  0.0f;
#endif

	this->id = id;

	// store the lighthouse positions
	this->lighthouse[0] = lh1;
	this->lighthouse[1] = lh2;

	this->xyz[0] = 0;
	this->xyz[1] = 0;
	this->xyz[2] = 0;
};


/*
 * Algoritm:
 *	http://geomalgorithms.com/a07-_distance.html#Distance-between-Lines
 */
static bool
intersect_lines(
	vec3d &orig1,
	vec3d &vec1,
	vec3d &orig2,
	vec3d &vec2,
	float res[3],
	float *dist
)
{
	vec3d w0 = {};
	arm_sub_f32(orig1, orig2, w0, vec3d_size);

	float a, b, c, d, e;
	arm_dot_prod_f32(vec1, vec1, vec3d_size, &a);
	arm_dot_prod_f32(vec1, vec2, vec3d_size, &b);
	arm_dot_prod_f32(vec2, vec2, vec3d_size, &c);
	arm_dot_prod_f32(vec1, w0, vec3d_size, &d);
	arm_dot_prod_f32(vec2, w0, vec3d_size, &e);

	float denom = a * c - b * b;
	if (fabs(denom) < 1e-5f)
		return false;

	// Closest point to 2nd line on 1st line
	float t1 = (b * e - c * d) / denom;
	vec3d pt1 = {};
	arm_scale_f32(vec1, t1, pt1, vec3d_size);
	arm_add_f32(pt1, orig1, pt1, vec3d_size);

	// Closest point to 1st line on 2nd line
	float t2 = (a * e - b * d) / denom;
	vec3d pt2 = {};
	arm_scale_f32(vec2, t2, pt2, vec3d_size);
	arm_add_f32(pt2, orig2, pt2, vec3d_size);

	// Result is in the middle
	vec3d tmp = {};
	arm_add_f32(pt1, pt2, tmp, vec3d_size);
	arm_scale_f32(tmp, 0.5f, res, vec3d_size);

	// Dist is distance between pt1 and pt2
	arm_sub_f32(pt1, pt2, tmp, vec3d_size);
	*dist = vec_length(tmp);

	return true;
}



// First 2 angles - x, y of station B; second 2 angles - x, y of station C.  Center is 4000. 180 deg = 8333.
// Y - Up;  X ->   Z v
// Station ray is inverse Z axis.
bool
LighthouseXYZ::compute()
{
	vec3d ray1 = {};
	vec3d ray2 = {};

	// translate the angle measurements of the Lighthouse sweeping
	// lasers into XYZ-space rays so that we can compute their
	// intersection in the real space.
	calc_ray_vec(this->lighthouse[0]->mat, this->angles[0], this->angles[1], ray1);

	calc_ray_vec(this->lighthouse[1]->mat, this->angles[2], this->angles[3], ray2);


	// compute the point of closest intersection between the two
	// XYZ-space rays coming from the lighthouses at their XYZ-space
	// positions.
	//
	// if the intersection isn't well defined (likely parallel?)
	// then don't update the position and signal an error.
	if (!intersect_lines(
		this->lighthouse[0]->origin, ray1,
		this->lighthouse[1]->origin, ray2,
		this->xyz,
		&this->dist
	))
		return false;

#if 0
	// Convert from the XYZ space of the lighthouses into NED
	// we don't need to do this?
	arm_matrix_instance_f32 pt_mat = {3, 1, this->xyz};
	arm_matrix_instance_f32 ned_mat = {3, 1, this->ned };
	arm_matrix_instance_f32 ned_rotation_mat = {3, 3, this->ned_rotation};

	arm_mat_mult_f32(&ned_rotation_mat, &pt_mat, &ned_mat);
#endif

	return true;
}


bool
LighthouseXYZ::update(
	unsigned ind,
	float angle
)
{
	if (ind >= 4)
		return false;

	this->angles[ind] = angle;
	this->fresh |= 1 << ind;
	if (this->fresh != 0xF)
		return false;
	this->fresh = 0;

	return this->compute();
}
