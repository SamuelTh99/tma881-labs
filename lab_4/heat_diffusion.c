#define CL_TARGET_OPENCL_VERSION 220

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <CL/cl.h>

void read_input_file(char* filename, size_t* rows, size_t* cols, float** matrix);
void init_cl(cl_context* context, cl_command_queue* command_queue, cl_program* program, cl_kernel* kernel);
char* read_program();
double average(float* matrix, size_t n);
double average_diff(float* matrix, double avg, size_t n);
void assert_success(cl_int error, char* msg);

#define FILENAME "diffusion"

int main(int argc, char* argv[]) {
    // Parse cmd args.
    float diffusion_constant = -1;
    long iterations = -1;

    int option; 
    while ((option = getopt(argc, argv, "n:d:")) != -1) {
        switch (option) {
            case 'n':
                iterations = atoi(optarg);
                break;
            case 'd':
                diffusion_constant = atof(optarg);
                break;
            default:
                printf("Usage: ./heat_diffusion -n<number of iterations> -d<diffusion constant>\n");
                return 1;
        }
    }

    char* filename = FILENAME;
    if (argc == 4) {
        filename = argv[3];
    }

    if (iterations  == -1 || diffusion_constant == -1){ 
        printf("Usage: ./heat_diffusion -n<number of iterations> -d<diffusion constant> <filename>\n");
        return 1;
    }

    // Read input file.
    size_t rows, cols;
    float* matrix;
    read_input_file(filename, &rows, &cols, &matrix);
    size_t n = rows * cols;

    // Init OpenCL.
    cl_context context;
    cl_command_queue command_queue; 
    cl_program program;
    cl_kernel kernel;
    init_cl(&context, &command_queue, &program, &kernel);

    // Create and init buffer.
    cl_int error;
    cl_mem matrix_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * n, NULL, &error);
    assert_success(error, "create cl buffer");
    error = clEnqueueWriteBuffer(command_queue, matrix_buffer, CL_TRUE, 0, sizeof(float) * n, matrix, 0, NULL, NULL);
    assert_success(error, "write to cl buffer");

    // Execute kernel.
    assert_success(clSetKernelArg(kernel, 0, sizeof(cl_mem), &matrix_buffer), "set kernel arg 0");
    assert_success(clSetKernelArg(kernel, 1, sizeof(cl_uint), &n), "set kernel arg 1");
    assert_success(clSetKernelArg(kernel, 2, sizeof(cl_uint), &cols), "set kernel arg 2");
    assert_success(clSetKernelArg(kernel, 3, sizeof(cl_float), &diffusion_constant), "set kernel arg 3");
    for (size_t i = 0; i < iterations; i++) {
        const size_t global = n;
        error = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, (const size_t*) &global, NULL, 0, NULL, NULL);
        assert_success(error, "enqueue kernel");
    }

    // Read output.
    error = clEnqueueReadBuffer(command_queue, matrix_buffer, CL_TRUE, 0, n * sizeof(float), matrix, 0, NULL, NULL);
    assert_success(error, "read from cl buffer");

    // Wait for computation to finish.
    error = clFinish(command_queue);
    assert_success(error, "finish");

    // Calculate & print averages.
    double avg = average(matrix, n); 
    double avg_diff = average_diff(matrix, avg, n);
    printf("average: %lf\naverage absolute difference: %lf\n", avg, avg_diff);

    // Release resources.
    free(matrix);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseMemObject(matrix_buffer);
}

void init_cl(cl_context* context, cl_command_queue* command_queue, cl_program* program, cl_kernel* kernel) {
    cl_int error;

    // Create platform.
    cl_platform_id platform_id;
    cl_uint nmb_platforms;
    error = clGetPlatformIDs(1, &platform_id, &nmb_platforms);
    assert_success(error, "get platform id");

    // Create device.
    cl_device_id device_id;
    cl_uint nmb_devices;
    error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &nmb_devices);
    assert_success(error, "get device id");

    // Create context.
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties) platform_id,
        0
    };
    *context = clCreateContext(properties, 1, &device_id, NULL, NULL, &error);
    assert_success(error, "create context");

    // Create command queue.
    *command_queue = clCreateCommandQueueWithProperties(*context, device_id, 0, &error);
    assert_success(error, "create command queue");
    
    // Build kernel.
    char* opencl_program_src = read_program();
    *program = clCreateProgramWithSource(*context, 1, (const char **) &opencl_program_src, NULL, &error);
    free(opencl_program_src);
    assert_success(error, "create program");

    error = clBuildProgram(*program, 0, NULL, NULL, NULL, NULL);
    if (error != CL_SUCCESS) {
        printf("cannot build program. log:\n");

        size_t log_size = 0;
        error = clGetProgramBuildInfo(*program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        assert_success(error, "get program build info");

        char* log = calloc(log_size, sizeof(char));
        if (log == NULL) {
            printf("could not allocate memory\n");
            exit(1);
        }

        error = clGetProgramBuildInfo(*program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        assert_success(error, "get program build info");
        printf( "%s\n", log );

        free(log);

        exit(1);
    }

    *kernel = clCreateKernel(*program, "heat_diffusion", &error);
    assert_success(error, "create kernel");
}

char* read_program() {
    FILE *f = fopen("heat_diffusion.cl", "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* string = malloc(fsize + 1);
    fread(string, 1, fsize, f);
    fclose(f);

    string[fsize] = 0;

    return string;
}

void read_input_file(char* filename, size_t* rows, size_t* cols, float** matrix) {
    FILE* f = fopen(filename, "r");
    char* line;
    int read = fscanf(f, "%zu %zu\n", cols, rows);
    if (read < 2) {
        printf("error read input file\n");
    }

    size_t n = (*rows) * (*cols);
    float* m = (float*) malloc(sizeof(float) * n);
    for (size_t i = 0; i < n; i++) {
        m[i] = 0;
    }

    size_t row, col;
    float val;
    while ((read = fscanf(f, "%zu %zu %f\n", &col, &row, &val)) == 3) {
        m[row * (*cols) + col] = val;
    }

    *matrix = m;
}

double average(float* matrix, size_t n) {
  double avg = 0;
  for (size_t i = 1; i <= n; i++) {
    double x = matrix[i-1];
    avg += (x - avg) / i;
  }
  return avg;
}

double average_diff(float* matrix, double avg, size_t n) {
  double avg_diff = 0;
  for (size_t i = 1; i <= n; i++) {
    double x = abs(matrix[i-1] - avg);
    avg_diff += (x - avg_diff) / i;
  }
  return avg_diff;
}

void assert_success(cl_int error, char* msg) {
    if (error != CL_SUCCESS) {
        printf("error: %s\n", msg);
        exit(1);
    }
}
