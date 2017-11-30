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

#ifdef __APPLE__
# include <OpenCL/cl.h>
# include "mac-mlx/mlx.h"
#endif

#ifndef __APPLE__
# include <CL/cl.h>
# include "linux-mlx/mlx.h"
#endif

#define UNIT_X (cl_float3){1, 0, 0}
#define UNIT_Y (cl_float3){0, 1, 0}
#define UNIT_Z (cl_float3){0, 0, 1}

#define ERROR 1e-4

#define BLACK (cl_float3){0.0, 0.0, 0.0}
#define RED (cl_float3){1.0, 0.2, 0.2}
#define GREEN (cl_float3){0.2, 1.0, 0.2}
#define BLUE (cl_float3){0.2, 0.2, 1.0}
#define WHITE (cl_float3){1.0, 1.0, 1.0}


#define GPU_MAT_DIFFUSE 1
#define GPU_MAT_SPECULAR 2
#define GPU_MAT_REFRACTIVE 3
#define GPU_MAT_NULL 4

#define GPU_SPHERE 1
#define GPU_TRIANGLE 3
#define GPU_QUAD 4

enum type {SPHERE, PLANE, CYLINDER, TRIANGLE};
enum mat {MAT_DIFFUSE, MAT_SPECULAR, MAT_REFRACTIVE, MAT_NULL};

typedef struct s_3x3
{
	cl_float3 row1;
	cl_float3 row2;
	cl_float3 row3;
}				t_3x3;

typedef struct s_camera
{
	cl_float3 center;
	cl_float3 normal;
	float width;
	float height;

	cl_float3 focus;
	cl_float3 origin;
	cl_float3 d_x;
	cl_float3 d_y;
}				t_camera;

typedef struct s_map
{
	int height;
	int width;
	cl_uchar *pixels;
}				Map;

typedef struct s_material
{
	char *friendly_name;
	float Ns; //specular exponent
	float Ni; //index of refraction
	float d; //opacity
	float Tr; //transparency (1 - d)
	cl_float3 Tf; //transmission filter (ie color)
	int illum; //flag for illumination model, raster only
	cl_float3 Ka; //ambient mask
	cl_float3 Kd; //diffuse mask
	cl_float3 Ks; //specular mask
	cl_float3 Ke; //emission mask

	char *map_Ka_path;
	Map *map_Ka;

	char *map_Kd_path;
	Map *map_Kd;

	char *map_bump_path;
	Map *map_bump;

	char *map_d_path;
	Map *map_d;
}				Material;

typedef struct s_face
{
	cl_int shape;
	cl_int mat_type;
	cl_int mat_ind;
	cl_int smoothing;
	cl_float3 verts[4];
	cl_float3 norms[4];
	cl_float3 tex[4];
	cl_float3 N;
}				Face;

typedef struct compact_box
{
	cl_float3 min;
	cl_float3 max;
	cl_int key;
}				C_Box;

typedef struct s_Box
{
	cl_float3 min; //spatial bounds of box
	cl_float3 max; 
	cl_int start; //indices in Faces[] that this box contains
	cl_int end;
	cl_int children[8];
	cl_int children_count;
}				Box;

typedef struct s_new_scene
{
	Material *materials;
	int mat_count;
	Face *faces;
	int face_count;
	Box *boxes;
	int box_count;
	C_Box *c_boxes;
	int c_box_count;
}				Scene;

void draw_pixels(void *img, int xres, int yres, cl_float3 *pixels);

void init_camera(t_camera *camera, int xres, int yres);

Scene *scene_from_obj(char *rel_path, char *filename);

cl_float3 *gpu_render(Scene *scene, t_camera cam, int xdim, int ydim);

void old_bvh(Scene *S);
Box *bvh_obj(Face *Faces, int start, int end, int *boxcount);
void gpu_ready_bvh(Scene *S, int *counts, int obj_count);




//vector helpers
float vec_mag(const cl_float3 vec);
cl_float3 unit_vec(const cl_float3 vec);
float dot(const cl_float3 a, const cl_float3 b);
cl_float3 cross(const cl_float3 a, const cl_float3 b);
cl_float3 vec_sub(const cl_float3 a, const cl_float3 b);
cl_float3 vec_add(const cl_float3 a, const cl_float3 b);
cl_float3 vec_scale(const cl_float3 vec, const float scalar);
cl_float3 mat_vec_mult(const t_3x3 mat, const cl_float3 vec);
cl_float3 angle_axis_rot(const float angle, const cl_float3 axis, const cl_float3 vec);
t_3x3 rotation_matrix(const cl_float3 a, const cl_float3 b);
cl_float3 vec_rev(cl_float3 v);


//utility functions
void print_vec(const cl_float3 vec);
void print_3x3(const t_3x3 mat);
void print_clf3(cl_float3 v);
float max3(float a, float b, float c);
float min3(float a, float b, float c);