#pragma once

#include <CL/cl.h>

static void __printKernelCompilationInfo(cl_program program, cl_device_id device_id)
{
	fprintf(stderr,"clBuildProgram failed\n");
	size_t length;
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &length);
	char *buffer = (char*)malloc(sizeof(char)*length);
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, length, buffer, NULL);
	fprintf(stderr,"CL_PROGRAM_BUILD_LOG: \n%s\n",buffer);
	free(buffer);
}

static void __get_platform_and_device()
{
	// Get platform and device information
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_DEFAULT, 1, 
					&device_id, &ret_num_devices);
}

static cl_context __get_context()
{
#ifdef RAY_GL
#ifdef __gnu_linux__
	// Create CL context properties, add GLX context & handle to DC
	cl_context_properties properties[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), // GLX Context
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), // GLX Display
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id, // OpenCL platform
		0
	};

	// Find CL capable devices in the current GL context
	cl_device_id devices[32]; size_t size;
	void *func = clGetExtensionFunctionAddress("clGetGLContextInfoKHR");
	((cl_int(*)(const cl_context_properties*,cl_gl_context_info,size_t,void*,size_t*))func)
	    (properties, CL_DEVICES_FOR_GL_CONTEXT_KHR, 32*sizeof(cl_device_id), devices, &size);
	
	// Create a context using the supported devices
	int count = size / sizeof(cl_device_id);
	return clCreateContext(properties, count, devices, NULL, 0, 0);
#endif // __gnu_linux__
#ifdef WIN32
	// Use WGL context
	return clCreateContext( NULL, 1, &device_id, NULL, NULL, &err);
#endif // WIN32
#else
	// Create an OpenCL context
	return clCreateContext( NULL, 1, &device_id, NULL, NULL, &err);
#endif // CLR_GL
}

static cl_mem __get_image()
{
	cl_int err;
#ifdef RAY_GL
	// Create shared image 
	GLenum 
	  gl_texture_target = GL_TEXTURE_2D,
	  gl_texture_internal = GL_RGBA32F,
	  gl_texture_format = GL_RGBA,
	  gl_texture_type = GL_FLOAT;
	
	// Create a texture in OpenGL and allocate space
	glGenTextures(1, &gl_texture_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(gl_texture_target, gl_texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(gl_texture_target, 0, gl_texture_internal, width, height, 0, gl_texture_format, gl_texture_type, NULL);
	glBindTexture(gl_texture_target, 0);
	
	// Create a reference mem object in OpenCL from GL texture
	return clCreateFromGLTexture2D(context, CL_MEM_READ_WRITE, gl_texture_target, 0, gl_texture_id, &err);
	if (!cl_image || err != CL_SUCCESS)
	{
		fprintf(stderr,"Failed to create OpenGL texture reference! %d\n", err);
		return (cl_mem)-1;
	}
#else // RAY_GL
	// create cl_image here
	return clCreateImage2D(context,CL_MEM_READ_WRITE,NULL,width,height,1,NULL,&err); //???
#endif // RAY_GL
}
