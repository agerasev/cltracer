#pragma once

#define RAY_GL

#include <CL/cl.h>
#ifdef RAY_GL
#include <GL/glew.h>
#endif

int  rayInit(int w, int h);
void rayDispose();

void raySetFov(float yfov);
void raySetSize(int w, int h);

void raySetPos(const float *pos);
void raySetOri(const float *ang);

void rayLoadGeometry(const float *geom, long size);
void rayLoadInstance(const float *map, const unsigned *index, long size);

int  rayRender();
void rayGetImage(float *data);
#ifdef RAY_GL
GLuint rayGetGLTexture();
#endif

