#pragma once

#include <CL/cl.h>
#include <GL/glew.h>

int initView(int w, int h);
void disposeView();

void setFov(float yfov);
void setSize(int w, int h);

void setCamPos(const float *pos);
void setCamOri(const float *ang);

void loadGeometry(const float *geom, long size);
void loadInstance(const float *map, const unsigned *index, long size);

void render();
GLuint getTexture();
