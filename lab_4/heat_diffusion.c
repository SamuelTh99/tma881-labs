#include <stdio.h>
#include <stdlib.h>
#include <OpenCL/opencl.h> // CL/cl.h on gantenbein

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





    // Release stuff.
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);
}
