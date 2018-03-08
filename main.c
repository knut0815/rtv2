#include "rt.h"

#ifndef __APPLE__
# define quit_key 113
#else
#endif

//THIS DOESNT WORK RIGHT, JUST FOR TESTING
#define H_FOV M_PI_2 * 60.0 / 90.0

void		set_camera(t_camera *cam)
{
	// printf("angle_x = %f\tangle_y = %f\n", cam->angle_x, cam->angle_y);
	// printf("CAM_DIR: ");
	// print_vec(cam->dir);

	cam->focus = vec_add(cam->pos, vec_scale(cam->dir, cam->dist));

	//now i need unit vectors on the plane
	t_3x3 rot_matrix;
	cl_float3 camera_x, camera_y;
	if (fabs(cam->dir.x) < .00001) //horizontal rotation
		camera_x = (cam->dir.z > 0) ? UNIT_X : vec_scale(UNIT_X, -1);
	else
	{
		// rot_matrix = rotation_matrix(UNIT_Z, (cl_float3){cam->dir.x, 0, cam->dir.z});
		// camera_x = mat_vec_mult(rot_matrix, UNIT_X);
		camera_x = unit_vec(cross((cl_float3){cam->dir.x, 0, cam->dir.z}, (cl_float3){0, -1, 0}));
	}
	if (fabs(cam->dir.y) < .00001) //vertical rotation
		camera_y = UNIT_Y;
	else
	{
		// rot_matrix = rotation_matrix((cl_float3){cam->dir.x, 0, cam->dir.z}, cam->dir);
		// camera_y = mat_vec_mult(rot_matrix, UNIT_Y);
		camera_y = unit_vec(cross(cam->dir, camera_x));
	}
	// if (camera_y.y < 0) //when vertical plane reference points down
	// 	camera_y.y *= -1;
	// print_vec(camera_x);
	// print_vec(camera_y);
	
	//pixel deltas
	cam->d_x = vec_scale(camera_x, cam->width / (float)XDIM);
	cam->d_y = vec_scale(camera_y, cam->height / (float)YDIM);

	//start at bottom corner (the plane's "origin")
	cam->origin = vec_sub(cam->pos, vec_scale(camera_x, cam->width / 2.0));
	cam->origin = vec_sub(cam->origin, vec_scale(camera_y, cam->height / 2.0));
}

t_camera	init_camera(void)
{
	t_camera	cam;
	//cam.pos = (cl_float3){-400.0, 50.0, -220.0}; //reference vase view (1,0,0)
	//cam.pos = (cl_float3){-540.0, 150.0, 380.0}; //weird wall-hole (0,0,1)
	cam.pos = (cl_float3){800.0, 450.0, 0.0}; //standard high perspective on curtain
	//cam.pos = (cl_float3){-800.0, 600.0, 350.0}; upstairs left
	//cam.pos = (cl_float3){800.0, 100.0, 350.0}; //down left
	//cam.pos = (cl_float3){900.0, 150.0, -35.0}; //lion
	//cam.pos = (cl_float3){-250.0, 100.0, 0.0};
	cam.dir = unit_vec((cl_float3){-1.0, 0.0, 0.0});
	cam.width = 1.0;
	cam.height = 1.0;
	cam.angle_x = atan2(cam.dir.z, cam.dir.x);
	cam.angle_y = 0;
	//determine a focus point in front of the view-plane
	//such that the edge-focus-edge vertex has angle H_FOV

	cam.dist = (cam.width / 2.0) / tan(H_FOV / 2.0);
	set_camera(&cam);
	return cam;
}

t_env		*init_env(Scene *S)
{
	t_env	*env = malloc(sizeof(t_env));
	env->cam = init_camera();
	env->pixels = malloc(sizeof(cl_double3) * ((XDIM) * (YDIM)));
	env->scene = S;
	env->mlx = mlx_init();
	env->win = mlx_new_window(env->mlx, XDIM, YDIM, "PATH_TRACER");
	env->img = mlx_new_image(env->mlx, XDIM, YDIM);
	env->mode = 1;
	env->show_fps = 0;
	env->key.w = 0;
	env->key.a = 0;
	env->key.s = 0;
	env->key.d = 0;
	env->key.uarr = 0;
	env->key.darr = 0;
	env->key.larr = 0;
	env->key.rarr = 0;
	env->key.space = 0;
	env->key.shift = 0;
	mlx_hook(env->win, 2, 0, key_press, env);
	mlx_hook(env->win, 3, 0, key_release, env);
	// mlx_hook(env->win, 6, 0, mouse_pos, env);
	mlx_hook(env->win, 17, 0, exit_hook, env);
	mlx_loop_hook(env->mlx, forever_loop, env);
	mlx_loop(env->mlx);
	return env;
}

int 		main(int ac, char **av)
{
	srand(time(NULL));
	t_camera cam;
	cam.pos = (cl_float3){-800, 450, 0};
	cam.dir = (cl_float3){1, 0, 0};
  //cam.center = (cl_float3){-400.0, 50.0, -220.0}; //reference vase view (1,0,0)
	//cam.center = (cl_float3){-540.0, 150.0, 380.0}; //weird wall-hole (0,0,1)
	//cam.center = (cl_float3){0, 650, 0}; //standard high perspective on curtain
	//cam.center = (cl_float3){-800.0, 600.0, 350.0}; upstairs left
	//cam.center = (cl_float3){800.0, 100.0, 350.0}; //down left
	//cam.center = (cl_float3){900.0, 150.0, -35.0}; //lion
	//cam.center = (cl_float3){-250.0, 100.0, 0.0};
  //	cam.normal = (cl_float3){1.0, 0.0, 0.0};
	cam.width = 1.0;
	cam.height = 1.0;
	cam.dist = (cam.width / 2.0) / tan(H_FOV / 2.0);
	unsigned int samples = 50;
	Scene *sponza = import_file(&cam, &samples);
  cam.angle_x = atan2(cam.dir.z, cam.dir.x);
	cam.angle_y = 0;

	//LL is best for this bvh. don't want to rearrange import for now, will do later
	Face *face_list = NULL;
	for (int i = 0; i < sponza->face_count; i++)
	{
		Face *f = calloc(1, sizeof(Face));
		memcpy(f, &sponza->faces[i], sizeof(Face));
		f->next = face_list;
		face_list = f;
	}
	free(sponza->faces);

	//Build BVH
	int box_count, ref_count;
	AABB *tree = sbvh(face_list, &box_count, &ref_count);
	printf("finished with %d boxes\n", box_count);
	study_tree(tree, 100000);

	//Flatten BVH
	sponza->bins = tree;
	sponza->bin_count = box_count;
	sponza->face_count = ref_count;
	printf("about to flatten\n");
	flatten_faces(sponza);
	printf("starting render\n");

// 	cl_double3 *pixels = gpu_render(sponza, cam, XDIM, YDIM, samples);

// 	void *mlx = mlx_init();
// 	void *win = mlx_new_window(mlx, XDIM, YDIM, "pathtracer");
// 	void *img = mlx_new_image(mlx, XDIM, YDIM);
// 	draw_pixels(img, XDIM, YDIM, pixels);
// 	mlx_put_image_to_window(mlx, win, img, 0, 0);

// // 	t_param *param = calloc(1, sizeof(t_param));
// 	*param = (t_param){mlx, win, img, XDIM, YDIM, sponza, cam, NULL, 0};
	
// 	mlx_key_hook(win, key_hook, param);
// 	mlx_loop(mlx);

	printf("all done\n");
  //Render
	t_env		*env = init_env(sponza);
	return 0;
}
