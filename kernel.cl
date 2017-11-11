
__constant float PI = 3.14159265359f;
__constant float DIFFUSE_CONSTANT = 0.7;
__constant int SAMPLES = 1024;
__constant int POOLSIZE = 256;
__constant int MAXDEPTH = 15;

#define SPHERE 1
#define TRIANGLE 2

#define MAT_DIFFUSE 1
#define MAT_SPECULAR 2
#define MAT_REFRACTIVE 3
#define MAT_NULL 4

#define BLACK (float3)(0.0f, 0.0f, 0.0f)
#define WHITE (float3)(1.0f, 1.0f, 1.0f)
#define GREY (float3)(0.5, 0.5, 0.5)

typedef struct s_ray {
	float3 origin;
	float3 direction;
}				Ray;

typedef struct s_object
{
	int shape;
	int material;
	float3 v0;
	float3 v1;
	float3 v2;
	float3 color;
	float3 emission;
}				Object;

typedef struct s_halton
{
	double value;
	double inv_base;
}				Halton;

typedef struct s_camera
{
	float3 origin;
	float3 focus;
	float3 d_x;
	float3 d_y;
}				Camera;

static float get_random(unsigned int *seed0, unsigned int *seed1) {

	/* hash the seeds using bitwise AND operations and bitshifts */
	*seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);  
	*seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

	unsigned int ires = ((*seed0) << 16) + (*seed1);

	/* use union struct to convert int to float */
	union {
		float f;
		unsigned int ui;
	} res;

	res.ui = (ires & 0x007fffff) | 0x40000000;  /* bitwise AND, bitwise OR */
	return (res.f - 2.0f) / 2.0f;
}

Halton setup_halton(int i, int base)
{
	Halton hal;
	double f = 1.0 / (double)base;
	hal.inv_base = f;
	hal.value = 0.0;
	while(i > 0)
	{
		hal.value += f * (double)(i % base);
		i /= base;
		f *= hal.inv_base;
	}
	return hal;
}

double next_hal(Halton *hal)
{
	double r = 1.0 - hal->value - 0.0000001;
	if (hal->inv_base < r)
		hal->value += hal->inv_base;
	else
	{
		double h = hal->inv_base;
		double hh;
		do {hh = h; h *= hal->inv_base;} while(h >= r);
		hal->value += hh + h - 1.0;
	}
	return hal->value;
}

int intersect_sphere(Ray ray, Object obj, float *t)
{
	float3 toCenter = obj.v0 - ray.origin;
	float b = dot(toCenter, ray.direction);
	float c = dot(toCenter, toCenter) - obj.v1.x * obj.v1.x;
	float disc = b * b - c;

	if (disc < 0.0f) return 0;
	else disc = sqrt(disc);

	if ((b - disc) > 0.00003f)
	{
		*t = b - disc;
		return 1;
	}
	if ((b + disc) > 0.00003f) 
	{
		*t = b + disc;
		return 1;
	}

	return 0;
}

int intersect_triangle(Ray ray, Object obj, float *t)
{
	float3 e1 = obj.v1 - obj.v0;
	float3 e2 = obj.v2 - obj.v0;

	float3 h = cross(ray.direction, e2);
	float a = dot(h, e1);

	if (fabs(a) < 0)
		return 0;
	float f = 1.0f / a;
	float3 s = ray.origin - obj.v0;
	float u = f * dot(s, h);
	if (u < 0.0 || u > 1.0)
		return 0;
	float3 q = cross(s, e1);
	float v = f * dot(ray.direction, q);
	if (v < 0.0 || u + v > 1.0)
		return 0;
	float this_t = f * dot(e2, q);
	if (this_t > 0)
	{
		*t = this_t;
		return (1);
	}
	else
		return (0);
}

int hit_nearest_brute(const Ray ray, __constant Object *scene, int object_count, float *t)
{
	float t_min = FLT_MAX;
	int best_ind = -1;
	for (int i = 0; i < object_count; i++)
	{
		float this_t = FLT_MAX;
		Object obj = scene[i];
		if (obj.shape == SPHERE)
		{
			if (intersect_sphere(ray, obj, &this_t) && this_t < t_min)
			{
				t_min = this_t;
				best_ind = i;
			}
		}
		else
		{
			if (intersect_triangle(ray, obj, &this_t) && this_t < t_min)
			{
				t_min = this_t;
				best_ind = i;
			}
		}
	}
	*t = t_min;
	return best_ind;
}

float3 trace(Ray ray, __constant Object *scene, int object_count, unsigned int *seed0, unsigned int *seed1)
{
	float3 color = (float3)(0.0f, 0.0f, 0.0f);
	float3 mask = (float3)(1.0f, 1.0f, 1.0f);
	for (int j = 0; j < MAXDEPTH; j++)
	{
		//collide
		float t;
		int hit_ind = hit_nearest_brute(ray, scene, object_count, &t);
		if (hit_ind == -1)
			break ;
		Object hit = scene[hit_ind];
		float3 hit_point = ray.origin + ray.direction * t;

		//get normal at collision point
		float3 N;
		if (hit.shape == SPHERE)
			N = normalize(hit_point - hit.v0);
		else
		{
			N = normalize(cross(hit.v1 - hit.v0, hit.v2 - hit.v0));
		}
		N = dot(ray.direction, N) < 0.0f ? N : N * (-1.0f);
		
		
		//local orthonormal system
		float3 axis = fabs(N.x) > fabs(N.y) ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
		float3 hem_x = normalize(cross(axis, N)); // I don't think normalize is necessary here (?)
		float3 hem_y = cross(N, hem_x);

		//generate random direction on the unit hemisphere
		float rsq = get_random(seed0, seed1);
		float r = sqrt(rsq);
		float theta = 2 * PI * get_random(seed0, seed1);

		//combine for new direction
		float3 rand_dir = normalize(hem_x * r * cos(theta) + hem_y * r * sin(theta) + N * sqrt(max(0.0f, 1.0f - rsq))); //again is normalize necessary?

		//consolidate
		ray.origin = hit_point + N * 0.00003f;
		ray.direction = rand_dir;
		color += mask * hit.emission;
		mask *= hit.color * dot(rand_dir, N) * DIFFUSE_CONSTANT;
	}
	return color;
}

Ray ray_from_cam(const Camera cam, float x, float y)
{
	Ray ray;
	ray.origin = cam.focus;
	float3 through = cam.origin + cam.d_x * x + cam.d_y * y;
	ray.direction = normalize(cam.focus - through);
	return ray;
}

__kernel void render_kernel(__constant Object *scene, 
							const float3 cam_origin,
							const float3 cam_focus,
							const float3 cam_dx,
							const float3 cam_dy, 
							const int width, 
							const int height, 
							const int obj_count,
							__local float3* sample_pool, 
							__global float3* output,
							__global uint* seeds)
{
	unsigned int pixel_id = get_group_id(0);
	unsigned int local_id = get_local_id(0);
	unsigned int x = pixel_id % width;
	unsigned int y = pixel_id / width;

	unsigned int xoff = local_id % 16;
	unsigned int yoff = local_id / 16;

	/* seeds for random number generator */
	unsigned int seed0 = seeds[pixel_id * 2] + get_global_id(0);
	unsigned int seed1 = seeds[pixel_id * 2 + 1];

	float x_coord = (float)x + (float)xoff / 16.0f;
	float y_coord = (float)y + (float)yoff / 16.0f;

	float3 sum_color = (float3)(0.0f, 0.0f, 0.0f);

	Camera cam;
	cam.origin = cam_origin;
	cam.focus = cam_focus;
	cam.d_x = cam_dx;
	cam.d_y = cam_dy;

	for (int i = 0; i < SAMPLES / POOLSIZE; i++)
	{
		Ray ray = ray_from_cam(cam, x_coord, y_coord);
		sum_color += trace(ray, scene, obj_count, &seed0, &seed1) / (float)SAMPLES;
	}

	sample_pool[local_id] = sum_color;
	barrier(CLK_LOCAL_MEM_FENCE);

	//there is a faster way to do this but that's for later
	if (local_id == 0)
	{
		for (int i = 1; i < POOLSIZE; i++)
			sum_color += sample_pool[i];
		output[pixel_id] = sum_color;
	}
}
