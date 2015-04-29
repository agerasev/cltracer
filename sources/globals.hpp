#pragma once

#define CAM_SIZE (27*sizeof(float))
#define RAY_SIZE (9*sizeof(float) + 2*sizeof(int))
#define HIT_SIZE (12*sizeof(float) + 3*sizeof(int))
#define HIT_INFO_SIZE (6*sizeof(int))

#define MAX_CHILD_RAYS 2

static cl_platform_id platform_id = 0;
static cl_device_id device_id = 0;

static cl_context context;
static cl_command_queue command_queue;

#ifdef RAY_GL
static GLuint gl_texture_id;
#endif
static cl_mem cl_image;

static cl_program program;

static unsigned int width, height;

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
