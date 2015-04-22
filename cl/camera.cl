typedef struct
{
	float3 pos;
	float3 ori[3]; // replace with float3x3 in later version
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
	cam.fov = data[12];
	cam.rad = data[13];
	cam.dof = data[14];
	return cam;
}
