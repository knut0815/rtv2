#pragma once

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <pthread.h>

#include "mlx/mlx.h"

#define UNIT_X (t_float3){1, 0, 0}
#define UNIT_Y (t_float3){0, 1, 0}
#define UNIT_Z (t_float3){0, 0, 1}

#define ERROR 1e-4

#define BLACK (t_float3){0.0, 0.0, 0.0}
#define RED (t_float3){1.0, 0.0, 0.0}
#define GREEN (t_float3){0.0, 1.0, 0.0}
#define BLUE (t_float3){0.0, 0.0, 1.0}
#define WHITE (t_float3){1.0, 1.0, 1.0}

enum type {SPHERE, PLANE, CYLINDER, TRIANGLE};

typedef struct s_float3
{
	float x;
	float y;
	float z;
}				t_float3;

typedef struct s_3x3
{
	t_float3 row1;
	t_float3 row2;
	t_float3 row3;
}				t_3x3;

typedef struct s_plane
{
	//keep this and turn it into t_camera
	t_float3 center;
	t_float3 normal;
	float width;
	float height; 
}				t_plane;

typedef struct s_object
{
	enum type shape;
	t_float3 position;
	t_float3 normal;
	t_float3 corner;
	t_float3 color;
	struct s_object *next;
}				t_object;

typedef struct s_ray
{
	t_float3 origin;
	t_float3 direction;
	t_float3 color;
}				t_ray;

typedef struct s_scene
{
	t_plane camera;
	t_float3 light;
	t_object *objects;
}				t_scene;

int hit(t_ray *ray, const t_scene *scene, int do_shade);
void draw_pixels(void *img, int xres, int yres, t_float3 *pixels);

void print_vec(const t_float3 vec);
void print_3x3(const t_3x3 mat);
void print_ray(const t_ray ray);
float vec_mag(const t_float3 vec);
t_float3 unit_vec(const t_float3 vec);
float dot(const t_float3 a, const t_float3 b);
t_float3 cross(const t_float3 a, const t_float3 b);
t_float3 vec_sub(const t_float3 a, const t_float3 b);
t_float3 vec_add(const t_float3 a, const t_float3 b);
t_float3 vec_scale(const t_float3 vec, const float scalar);
t_float3 mat_vec_mult(const t_3x3 mat, const t_float3 vec);
t_float3 angle_axis_rot(const float angle, const t_float3 axis, const t_float3 vec);
t_3x3 rotation_matrix(const t_float3 a, const t_float3 b);
t_float3 bounce(const t_float3 in, const t_float3 norm);
float dist(const t_float3 a, const t_float3 b);
t_float3 vec_rev(t_float3 v);

int intersect_object(const t_ray *ray, const t_object *object, t_float3 *hit);
t_float3 norm_object(const t_object *object, const t_ray *ray);

void new_sphere(t_scene *scene, float x, float y, float z, float r, t_float3 color);
void new_plane(t_scene *scene, t_float3 center, t_float3 normal, t_float3 corner, t_float3 color);
void new_cylinder(t_scene *scene, t_float3 center, t_float3 radius, t_float3 extent, t_float3 color);
void new_triangle(t_scene *scene, t_float3 center, t_float3 normal, t_float3 corner, t_float3 color);


int intersect_triangle(const t_ray *ray, const t_object *triangle, t_float3 *hit);