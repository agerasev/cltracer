#pragma once

#define CAM_SIZE 27

#define RAY_FSIZE 9
#define RAY_ISIZE 2

#define HIT_FSIZE 12
#define HIT_ISIZE 3

#define HIT_INFO_SIZE 10

#define MAX_CHILD_RAYS 2

static cl_platform_id platform_id = 0;
static cl_device_id device_id = 0;

static cl_context context;
static cl_command_queue command_queue;

#ifdef RAY_GL
static GLuint gl_texture_id;
#endif
static cl_mem cl_image;
/*
static cl_mem cam_fdata;
static cl_mem ray_fdata, ray_idata;
static cl_mem hit_fdata, hit_idata;
static cl_mem color_buffer, accum_buffer;
static cl_mem hit_info, cl_ray_count;
static cl_mem pitch, work_size;
static cl_mem number, factor;
static cl_mem cl_random;
*/

static cl_program program;
//static cl_kernel start, intersect, produce, draw, compact;

static int width, height;

static float cam_pos[3] = {0.0f,0.0f,0.0f};
static float cam_ori[9] = 
{
  1,0,0,
  0,0,1,
  0,1,0
};
static float cam_pre_pos[3] = {0.0f,0.0f,0.0f};
static float cam_pre_ori[9] = 
{
  1,0,0,
  0,0,1,
  0,1,0
};
static float cam_fov = 1.0f;
static float cam_rad = 0.01f;
static float cam_dof = 4.0f;

static unsigned long samples = 0;

size_t screen_size;
size_t buffer_size;
