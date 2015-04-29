/** camera.cl */
#define CAM_SIZE 27

typedef struct
{
	float3 pos;
	float3 ori[3]; // replace with float3x3 in newer version
	float3 pre_pos;
	float3 pre_ori[3];
	float fov;
	float rad;
	float dof;
} Camera;

Camera camera_load(__constant const float *data)
{
	Camera cam;
	cam.pos = vload3(0,data);
	cam.ori[0] = vload3(1,data);
	cam.ori[1] = vload3(2,data);
	cam.ori[2] = vload3(3,data);
	cam.pre_pos = vload3(4,data);
	cam.pre_ori[0] = vload3(5,data);
	cam.pre_ori[1] = vload3(6,data);
	cam.pre_ori[2] = vload3(7,data);
	cam.fov = data[24];
	cam.rad = data[25];
	cam.dof = data[26];
	return cam;
}
