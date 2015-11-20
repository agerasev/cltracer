#include "gl.h"

#include <GL/glew.h>

#include <stdlib.h>
#include <stdio.h>

static void __printShaderCompilationErrors(GLuint id, const char *name)
{
	int   infologLen   = 0;
	int   charsWritten = 0;
	char *infoLog;
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infologLen);
	if(infologLen > 1)
	{
		infoLog = (char*)malloc(sizeof(char)*infologLen);
		glGetShaderInfoLog(id, infologLen, &charsWritten, infoLog);
		fprintf(stderr,"%s:\n%s\n",name,infoLog);
		free(infoLog);
	}
}

static char *__loadSource(const char *fn)
{
	FILE *fp;
	size_t size;
	char *source = NULL;
	
	fp = fopen(fn,"r");
	if(!fp)
	{
			fprintf(stderr,"Failed to load shader: %s\n",fn);
			return NULL;
	}
	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	source = (char*)malloc(sizeof(char)*(size+1));
	fread(source,1,size,fp);
	source[size] = '\0';
	fclose(fp);
	return source;
}

static void __freeSource(char *source)
{
	free(source);
}

// Global variables
static GLuint buffer = 0;
static char *source_vert = NULL;
static char *source_frag = NULL;
static GLuint shader_vert = 0;
static GLuint shader_frag = 0;
static GLuint program = 0;

void initGL()
{
	// Load buffer
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	float buffer_data[12] = {
		1.0f,1.0f,
		-1.0f,1.0f,
		-1.0f,-1.0f,
		1.0f,1.0f,
		-1.0f,-1.0f,
		1.0f,-1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, 2*3*2*sizeof(float), buffer_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	// Load, create and compile shaders
	source_vert = __loadSource("opengl_shaders/shader.vert");
	shader_vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader_vert, 1, (const GLchar**)&source_vert, NULL);
	glCompileShader(shader_vert);
	__printShaderCompilationErrors(shader_vert,"shader.vert");
	__freeSource(source_vert);
	source_frag = __loadSource("opengl_shaders/shader.frag");
	shader_frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader_frag, 1, (const GLchar**)&source_frag, NULL);
	glCompileShader(shader_frag);
	__printShaderCompilationErrors(shader_frag,"shader.frag");
	__freeSource(source_frag);
	
	// Create program, attach shaders and link program
	program = glCreateProgram();
	glAttachShader(program, shader_vert);
	glAttachShader(program, shader_frag);
	glLinkProgram(program);
}

void drawGLTexture(GLuint texture)
{
	// Draw texture
	glUseProgram(program);
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,texture);
		glUniform1i(glGetUniformLocation(program,"uTexture"),0);
		
		glEnableVertexAttribArray(glGetAttribLocation(program,"aVertex"));
		{
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glVertexAttribPointer(glGetAttribLocation(program,"aVertex"), 2, GL_FLOAT, GL_FALSE, 0, NULL);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLES,0,6);
		}
		glDisableVertexAttribArray(glGetAttribLocation(program,"aVertex"));
	}
	glUseProgram(0);
}

void disposeGL()
{
	// Delete program
	glDetachShader(program, shader_vert);
	glDetachShader(program, shader_frag);
	glDeleteProgram(program);
	
	// Delete shaders
	glDeleteShader(shader_vert);
	glDeleteShader(shader_frag);
	
	// Delete buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &buffer);
}
