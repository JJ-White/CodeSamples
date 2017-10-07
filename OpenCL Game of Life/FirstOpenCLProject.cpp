#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <CL/cl.h>
#include <GL/glut.h>

#include "bitmap_image.h"
#include "opencl_utils.h"
#include "OpenGL_functions.h"

#include <windows.h>

#define MAX_SOURCE_SIZE (0x100000)

#define CPU false
#define WIDTH 32
#define HEIGHT 32
#define LOCALWIDTH 16
#define LOCALHEIGHT 16
#define KERNEL "gameOfLife"
#define DEAD 0;
#define LIFE 1;

cl_kernel kernel = NULL;
cl_command_queue command_queue = NULL;
cl_mem ImageOnDevice = NULL;
cl_mem gridAOnDevice = NULL;
cl_mem gridBOnDevice = NULL;
LARGE_INTEGER freq, startGPU, endGPU;

int previous = -1;
int iteration = 0;
double avgTime = 30;

int pos(int x, int y) {
	return y * (WIDTH + 2) + x;
}

void display() {
	QueryPerformanceCounter(&startGPU);
	glFinish();
	clEnqueueAcquireGLObjects(command_queue, 1, &ImageOnDevice, 0, NULL, NULL);

	/* Set kernel arguments and run kernel */
	//Arguments are switched to made output of iteration 1 input of iteration to etc.
	bool even = iteration % 2 == 0;
	int ret = clSetKernelArg(kernel, even ? 0 : 1, sizeof(cl_mem), (void *)&gridAOnDevice);
	printError(ret);
	ret = clSetKernelArg(kernel, even ? 1 : 0, sizeof(cl_mem), (void *)&gridBOnDevice);
	printError(ret);

	size_t globalSize[] = { WIDTH, HEIGHT };
	size_t localSize[] = { LOCALWIDTH, LOCALHEIGHT };
	ret = clEnqueueNDRangeKernel(
		command_queue,
		kernel,
		2,
		NULL,
		globalSize,
		localSize,
		0,
		NULL,
		NULL
	);
	printError(ret);

	/*int* grid = new int[WIDTH * HEIGHT];
	ret = clEnqueueReadBuffer(
		command_queue,
		gridAOnDevice,
		CL_TRUE,
		0,
		sizeof(int),
		grid,
		0,
		NULL,
		NULL
		);
	printError(ret);
	delete grid;*/

	ret = clEnqueueReleaseGLObjects(command_queue, 1, &ImageOnDevice, 0, NULL, NULL);
	printError(ret);

	ret = clFinish(command_queue);
	printError(ret);

	/* Draw quad */
	draw_quad();
	glFlush();
	glutPostRedisplay();

	iteration++;
	QueryPerformanceCounter(&endGPU);
	double gpuTime = (double)(endGPU.QuadPart - startGPU.QuadPart) / freq.QuadPart * 1000.0;
	avgTime = (avgTime * 49 + gpuTime) / 50;
	printf("%.3f\n", avgTime);
	//printf("GPU time for 1 frame at %iX%i with workgroup size %iX%i is %.3f msec\n", WIDTH, HEIGHT, LOCALWIDTH, LOCALHEIGHT, gpuTime);
	//getchar();
}

void cpuGameOfLife(int* gridA) {
	int* gridB = new int[WIDTH * HEIGHT];
	LARGE_INTEGER startCPU, endCPU;
	while (true) {
		QueryPerformanceCounter(&startCPU);
		for (int posY = 0; posY < HEIGHT; posY++) {
			for (int posX = 0; posX < WIDTH; posX++) {
				int pos = posY * WIDTH + posX;
				int neighbors = 0;
				int mWidth = WIDTH - 1;
				int mHeight = HEIGHT - 1;

				if (posX > 0 && posY > 0 && gridA[pos - WIDTH - 1]) neighbors++;
				if (posY > 0 && gridA[pos - WIDTH]) neighbors++;
				if (posX < mWidth && posY > 0 && gridA[pos - WIDTH + 1]) neighbors++;
				if (posX > 0 && gridA[pos - 1]) neighbors++;
				if (posX < mWidth && gridA[pos + 1]) neighbors++;
				if (posX > 0 && posY < mHeight && gridA[pos + WIDTH - 1]) neighbors++;
				if (posY < mHeight && gridA[pos + WIDTH]) neighbors++;
				if (posX < mWidth && posY < mHeight && gridA[pos + WIDTH + 1]) neighbors++;

				//Determine fate
				int fate = gridA[pos];
				if ((fate && (neighbors == 2 || neighbors == 3)) || (!fate && neighbors == 3)) {
					fate = LIFE;
				}
				else {
					fate = DEAD;
				}

				//Write result to output grid
				gridB[pos] = fate;
			}
		}

		//Switch input and output arrays
		int* t = gridA;
		gridA = gridB;
		gridB = t;

		QueryPerformanceCounter(&endCPU);
		double cpuTime = (double)(endCPU.QuadPart - startCPU.QuadPart) / freq.QuadPart * 1000.0;
		printf("CPU time for 1 frame at %iX%i is %.3f msec\n", WIDTH, HEIGHT, cpuTime);
		getchar();
	}
}

int main(int argc, char** argv)
{
	QueryPerformanceFrequency(&freq);

	/* GLUT/OpenGL initialization */
	glutInit(&argc, argv);
	glutInitWindowSize(800, 800);
	glutCreateWindow("Game of Life - Kim Jooss & Juriaan Moonen");
	glutDisplayFunc(display);
	GLuint texture = init_gl(WIDTH, HEIGHT);
	
	/* OpenCL variable declarations */
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_program program = NULL;
	cl_kernel colorTableKernel = NULL;
	cl_platform_id platform_id = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;

	/* Get Platform and Device Info */
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	printError(ret);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
	printError(ret);

	/* Create OpenCL context */
	cl_context_properties properties[] = { 
		CL_GL_CONTEXT_KHR,         
		reinterpret_cast<cl_context_properties>(wglGetCurrentContext()),        
		CL_WGL_HDC_KHR,         
		reinterpret_cast<cl_context_properties>(wglGetCurrentDC()),        
		0 
	};
	context = clCreateContext(properties, 1, &device_id, NULL, NULL, &ret);
	printError(ret);

	/* Create Command Queue */
	command_queue = clCreateCommandQueue(context, device_id, NULL, &ret);
	printError(ret);

	/* Allocate memory for arrays on the Compute Device */
	ImageOnDevice = clCreateFromGLTexture(
		context, 
		CL_MEM_WRITE_ONLY, 
		GL_TEXTURE_2D, 
		0, 
		texture, 
		&ret);
	printError(ret);

	gridAOnDevice = clCreateBuffer(
		context,
		CL_MEM_READ_WRITE,
		(WIDTH + 2) * (HEIGHT + 2) * sizeof(int),
		NULL,
		&ret
	);
	printError(ret);

	gridBOnDevice = clCreateBuffer(
		context,
		CL_MEM_READ_WRITE,
		(WIDTH + 2) * (HEIGHT + 2) * sizeof(int),
		NULL,
		&ret
		);
	printError(ret);

	/* Copy initial grid configuration */
	int* grid = new int[(WIDTH + 2) * (HEIGHT + 2)];

	//Initialize empty grid

	for (int i = 0; i < (WIDTH + 2) * (HEIGHT + 2); i++) {
		grid[i] = DEAD;
	}

	//Add 10 cell row as starting condition
	grid[pos(5, 5)] = LIFE;
	grid[pos(6, 5)] = LIFE;
	grid[pos(7, 5)] = LIFE;
	grid[pos(8, 5)] = LIFE;
	grid[pos(9, 5)] = LIFE;
	grid[pos(10, 5)] = LIFE;
	grid[pos(11, 5)] = LIFE;
	grid[pos(12, 5)] = LIFE;
	grid[pos(13, 5)] = LIFE;
	grid[pos(14, 5)] = LIFE;

	ret = clEnqueueWriteBuffer(
		command_queue,
		gridAOnDevice,
		CL_TRUE,
		0,
		(WIDTH + 2) * (HEIGHT + 2) * sizeof(int),
		grid,
		0,
		NULL,
		NULL
	);
	printError(ret);
	delete grid;

	/* Build Kernel Program */
	char fileName[] = "./kernel.cl";
	program = build_program(context, device_id, fileName);

	/* Create kernel */
	kernel = clCreateKernel(program, KERNEL, &ret);
	printError(ret);

	ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&ImageOnDevice);
	printError(ret);

	/* GLUT main loop */
	if(!CPU) glutMainLoop();

	/* CPU Game of Life */
	if(CPU) cpuGameOfLife(grid);

	/* OpenCL finalization */
	ret = clFlush(command_queue);
	printError(ret);
	ret = clReleaseKernel(colorTableKernel);
	printError(ret);
	ret = clReleaseKernel(kernel);
	printError(ret);
	ret = clReleaseProgram(program);
	printError(ret);
	ret = clReleaseMemObject(ImageOnDevice);
	printError(ret);
	ret = clReleaseCommandQueue(command_queue);
	printError(ret);
	ret = clReleaseContext(context);
	printError(ret);

	return 0;
}