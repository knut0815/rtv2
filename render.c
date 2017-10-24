#include "rt.h"

//completely arbitrary values
#define ka 0.5
#define kd 2
#define ks 1

#define Ia 1



#define H_FOV M_PI_2 * 60.0 / 90.0 	//60 degrees. eventually this will be dynamic with aspect
									// V_FOV is implicit with aspect of view-plane

t_ray *rays_from_camera(t_camera camera, int xres, int yres)
{
	//xres and yres will likely end up in some struct.

	t_ray *rays = calloc(xres * yres, sizeof(t_ray));

	//determine a focus point in front of the view-plane
	//such that the edge-focus-edge vertex has angle H_FOV
	float d = (camera.width / 2.0) / tan(H_FOV / 2.0);
	t_float3 focus = vec_add(camera.center, vec_scale(camera.normal, d));

	//now i need unit vectors on the plane
	t_3x3 camera_matrix = rotation_matrix(UNIT_Z, camera.normal);
	t_float3 camera_x, camera_y;
	camera_x = mat_vec_mult(camera_matrix, UNIT_X);
	camera_y = mat_vec_mult(camera_matrix, UNIT_Y);

	//pixel deltas
	t_float3 delta_x, delta_y;
	delta_x = vec_scale(camera_x, camera.width / (float)xres);
	delta_y = vec_scale(camera_y, camera.height / (float)yres);

	//start at bottom corner (the plane's "origin")
	t_float3 origin = camera.center;
	origin = vec_sub(origin, vec_scale(camera_x, camera.width / 2.0));
	origin = vec_sub(origin, vec_scale(camera_y, camera.height / 2.0));

	for (int y = 0; y < yres; y++)
	{
		t_float3 from = origin;
		for (int x = 0; x < xres; x++)
		{
			rays[y * xres + x] = (t_ray){focus, unit_vec(vec_sub(focus, from)), BLACK, vec_inv(unit_vec(vec_sub(focus, from)))};
			from = vec_add(from, delta_x);
		}
		origin = vec_add(origin, delta_y);
	}

	return (rays);
}

#define REFLECTIVE 0.95

void trace(t_ray *ray, const t_scene *scene, int depth)
{
	if (depth > 10)
	{
		ray->color = BLACK;
		return ;
	}

	float closest_dist = FLT_MAX;
	t_object *closest_object = NULL;
	hit_nearest(ray, scene->bvh, &closest_object, &closest_dist);

	if (closest_object == NULL)
	{
		ray->color = BLACK;
		return;
	}


	t_float3 intersection = vec_add(ray->origin, vec_scale(ray->direction, closest_dist));
	ray->origin = intersection;
	t_float3 N = norm_object(closest_object, ray);
	
	ray->direction = bounce(ray->direction, N);
	ray->inv_dir = vec_inv(ray->direction);

	float emission = closest_object->emission;
	if (emission == 0.0)
		trace(ray, scene, depth + 1);
	
	ray->color = vec_add(ray->color, (t_float3){emission, emission, emission});
	ray->color = vec_scale(ray->color, REFLECTIVE);
	ray->color.x = fmax(0.0, fmin(1.0, ray->color.x));
	ray->color.y = fmax(0.0, fmin(1.0, ray->color.y));
	ray->color.z = fmax(0.0, fmin(1.0, ray->color.z));
}

t_float3 *simple_render(const t_scene *scene, const int xres, const int yres)
{
	clock_t start, end;
	start = clock();
	t_float3 *pixels = calloc(xres * yres, sizeof(t_float3));
	t_ray *rays = rays_from_camera(scene->camera, xres, yres);
	for (int y = 0; y < yres; y++)
		for (int x = 0; x < xres; x++)
		{
			//printf("tracing\n");
			trace(&(rays[y * xres + x]), scene, 0);
			pixels[y * xres + x] = rays[y * xres + x].color;
			//printf("traced\n");
		}
	end = clock();
	double cpu_time = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("trace time %lf sec, %lf FPS\n", cpu_time, 1.0 / cpu_time);
	free(rays);
	return pixels;
}

/*
notes:
	should fix planes and spheres asap
		-I should do this before making major changes it's going to get way harder to fix later
	next milestone:
		lights are solid objects
		rewrite trace/hit for recursive
		better implementation of color
*/