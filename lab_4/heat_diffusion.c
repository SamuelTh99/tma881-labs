#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

int main() {
    cl_int error;

    // Create platform.
    cl_platform_id platform_id;
    cl_uint nmb_platforms;
    if (clGetPlatformIDs(1, &platform_id, &nmb_platforms) != CL_SUCCESS) {
        printf( "cannot get platform\n" );
        return 1;
    }

    // Create device.
    cl_device_id device_id;
    cl_uint nmb_devices;
    if (clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1,
        &device_id, &nmb_devices) != CL_SUCCESS) {
        printf( "cannot get device\n" );
        return 1;
    }

    // Create context.
    cl_context context;
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties) platform_id,
        0
    };
    context = clCreateContext(properties, 1, &device_id, NULL, NULL, &error);

    // Create command queue.
    cl_command_queue command_queue;
    command_queue = clCreateCommandQueue(context, device_id, 0, &error);
    if (error != CL_SUCCESS) {
        printf("cannot create context\n");
        return 1;
    }

    // Build kernel.
    char* opencl_program_src =
"__kernel void\n\
    dot_prod_mul(\n\
    __global const float * a,\n\
    __global const float * b,\n\
    __global float * c\n\
    )\n\
    {\n\
    int ix = get_global_id(0);\n\
    c[ix] = a[ix] * b[ix];\n\
}";
    cl_program program;
    program = clCreateProgramWithSource(context,1,
            (const char **) &opencl_program_src, NULL, &error);

    clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

    cl_kernel kernel;
    kernel = clCreateKernel(program, "dot_prod_mul", &error);

    // Create and init buffers.
    const size_t ix_m = 1000;
    cl_mem input_buffer_a, input_buffer_b, output_buffer_c;
    input_buffer_a  = clCreateBuffer(context, CL_MEM_READ_ONLY,
                        sizeof(float) * ix_m, NULL, &error);
    input_buffer_b  = clCreateBuffer(context, CL_MEM_READ_ONLY,
                        sizeof(float) * ix_m, NULL, &error);
    output_buffer_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                        sizeof(float) * ix_m, NULL, &error);

    float * a = malloc(ix_m*sizeof(float));
    float * b = malloc(ix_m*sizeof(float));
    for (size_t ix; ix < ix_m; ++ix) {
        a[ix] = ix;
        b[ix] = ix;
    }
    clEnqueueWriteBuffer(command_queue, input_buffer_a, CL_TRUE,
        0, ix_m*sizeof(float), a, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queue, input_buffer_b, CL_TRUE,
        0, ix_m*sizeof(float), b, 0, NULL, NULL);


    // Execute kernel.
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer_a);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &input_buffer_b);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &output_buffer_c);

    const size_t global = ix_m;
    clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
        (const size_t *)&global, NULL, 0, NULL, NULL);

    // Read output.
    float * c = malloc(ix_m*sizeof(float));
    clEnqueueReadBuffer(command_queue, output_buffer_c, CL_TRUE,
        0, ix_m*sizeof(float), c, 0, NULL, NULL);

    clFinish(command_queue);

    printf("Square of %d is %f\n", 100, c[100]);

    // Release resources.
    free(a);
    free(b);
    free(c);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseMemObject(input_buffer_a);
    clReleaseMemObject(input_buffer_b);
    clReleaseMemObject(output_buffer_c);
}
