#pragma once

#include <CL/cl.h>
#ifdef RAY_GL
#include <GL/glew.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int  rayInit(int w, int h);
void rayDispose();

void raySetFov(float yfov);
void raySetDof(float rad, float dof);
void raySetSize(int w, int h);

void raySetPos(const float *pos);
void raySetOri(float yaw, float pitch);

void rayLoadGeometry(const float *geom, size_t size);
void rayLoadInstance(const float *map, const unsigned *index, long size);

void rayClear();
void rayUpdateMotion();
int  rayRender();
void rayGetImage(float *data);
#ifdef RAY_GL
GLuint rayGetGLTexture();
#endif

#ifdef __cplusplus
}
#endif
