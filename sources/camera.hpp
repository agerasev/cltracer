#pragma once

void raySetFov(float yfov)
{
	cam_fov = yfov;
}

void raySetDof(float rad, float dof)
{
	cam_rad = rad;
	cam_dof = dof;
}

void raySetSize(int w, int h)
{
	
}

static void __move_cam_pos()
{
	cam_pre_pos[0] = cam_pos[0];
	cam_pre_pos[1] = cam_pos[1];
	cam_pre_pos[2] = cam_pos[2];
}

void raySetPos(const float *pos)
{
	__move_cam_pos();
	
	cam_pos[0] = pos[0];
	cam_pos[1] = pos[1];
	cam_pos[2] = pos[2];
}

static void __move_cam_ori()
{
	cam_pre_ori[0] = cam_ori[0];
	cam_pre_ori[1] = cam_ori[1];
	cam_pre_ori[2] = cam_ori[2];
	
	cam_pre_ori[3] = cam_ori[3];
	cam_pre_ori[4] = cam_ori[4];
	cam_pre_ori[5] = cam_ori[5];
	
	cam_pre_ori[6] = cam_ori[6];
	cam_pre_ori[7] = cam_ori[7];
	cam_pre_ori[8] = cam_ori[8];
}

void raySetOri(float yaw, float pitch)
{
	__move_cam_ori();
	
	cam_ori[0] = cos(yaw);
	cam_ori[1] = sin(yaw);
	cam_ori[2] = 0.0f;
	
	cam_ori[3] = sin(yaw)*sin(pitch);
	cam_ori[4] = -cos(yaw)*sin(pitch);
	cam_ori[5] = cos(pitch);
	
	cam_ori[6] = -sin(yaw)*cos(pitch);
	cam_ori[7] = cos(yaw)*cos(pitch);
	cam_ori[8] = sin(pitch);
}
