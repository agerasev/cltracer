#pragma once

#include <GL/glew.h>

#ifdef __cplusplus
extern "C" {
#endif

void initGL();
void drawGLTexture(GLuint texture);
void disposeGL();

#ifdef __cplusplus
}
#endif
