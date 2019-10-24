#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <mpi.h>

void main_master(int nmb_mpi_proc, char* filename, long iterations, double diffusion_constant);
void main_worker(int nmb_mpi_proc, int mpi_rank, long iterations, double diffusion_constant);
void read_input_file(char* filename, size_t* rows, size_t* cols, double** matrix);
double apply_heat_diffusion(double* m, long i, long row_len, long n, double c);
void assert_success(int error, char* msg);
void print_matrix(double* matrix, long matrix_len);
double average(double* matrix, size_t n);
double average_diff(double* matrix, double avg, size_t n);

#define MASTER_RANK 0
#define FILENAME "diffusion"
#define RESULT_TAG 1

int main(int argc, char* argv[]) {
    // Parse cmd args.
    double diffusion_constant = -1;
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

    // Init MPI.
    int error;
    error = MPI_Init(&argc, &argv);
    assert_success(error, "mpi init");

    int nmb_mpi_proc, mpi_rank;
    error = MPI_Comm_size(MPI_COMM_WORLD, &nmb_mpi_proc);
    assert_success(error, "mpi comm size");
    error = MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    assert_success(error, "mpi comm rank");

    if (mpi_rank == 0) {
        main_master(nmb_mpi_proc, filename, iterations, diffusion_constant);
    } else {
        main_worker(nmb_mpi_proc, mpi_rank, iterations, diffusion_constant);
    }

    // Release resources.
    error = MPI_Finalize();
    assert_success(error, "finalize");
}

void main_master(int nmb_mpi_proc, char* filename, long iterations, double diffusion_constant) {
    // KANBAN BOARD:
    // x. Read file
    // x. Broadcast dimensions to all workers
    // 3. For each iteration:
    //    3.1. Broadcast matrix
    //    3.2. Wait for results to be sent back
    //    3.3. Update matrix
    // 4. Calculate averages
    // 5. Print results

    int error;

    // Read input file.
    long rows, cols;
    double* matrix;
    read_input_file(filename, &rows, &cols, &matrix);
    long matrix_len = rows * cols;
    long chunk_size = matrix_len / (nmb_mpi_proc - 1);

    if (nmb_mpi_proc == 1) {
        double* m = (double*) malloc(sizeof(double) * matrix_len);
        for (long iter = 0; iter < iterations; iter++) {
            for (long i = 0; i < matrix_len; i++) {
                m[i] = apply_heat_diffusion(matrix, i, cols, matrix_len, diffusion_constant);
            }
            double* tmp = m;
            m = matrix;
            matrix = tmp;
        }
    } else { 
        {
            long msg[2];
            msg[0] = cols;
            msg[1] = matrix_len;
            int len = 2;
            // printf("master: broadcasting dimensions\n");
            error = MPI_Bcast(msg, len, MPI_LONG, MASTER_RANK, MPI_COMM_WORLD);
            assert_success(error, "master bcast");
            // printf("master: done\n");
        }

        for (long iter = 0; iter < iterations; iter++) {
            // printf("master: broadcasting matrix\n");
            error = MPI_Bcast(matrix, matrix_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
            assert_success(error, "master bcast");
            // printf("master: done\n");

            for (long rank = 1; rank < nmb_mpi_proc; rank++) {
                // printf("master: receiving results from %d\n", rank);
                double* result = (double*) malloc(sizeof(double) * chunk_size);
                error = MPI_Recv(result, chunk_size, MPI_DOUBLE, rank, RESULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                assert_success(error, "master recv result");
                // printf("master: done\n");

                long offset = chunk_size * (rank - 1);
                for (long i = 0; i < chunk_size; i++) {
                    matrix[i + offset] = result[i];
                }
            }
        }
    }

    // Calculate & print averages.
    double avg = average(matrix, matrix_len); 
    double avg_diff = average_diff(matrix, avg, matrix_len);
    printf("average: %lf\naverage absolute difference: %lf\n", avg, avg_diff);
}

void main_worker(int nmb_mpi_proc, int mpi_rank, long iterations, double diffusion_constant) {
    // KANBAN BOARD:
    // x. Receive matrix dimensions
    // 1. For each iteration:
    //    1.1. Receive matrix
    //    1.2. For each index in range (calculated from rank):
    //         1.2.1. Calculate heat diffusion
    //         1.2.2. Store result in local result matrix
    //    1.3. Send result matrix to master

    int error;
    long row_len, matrix_len;

    {
        long msg[2];
        int len = 2;
        // printf("worker %d: receiving dimensions\n", mpi_rank);
        error = MPI_Bcast(msg, len, MPI_LONG, MASTER_RANK, MPI_COMM_WORLD);
        assert_success(error, "worker receive dimensions");
        // printf("worker %d: done\n", mpi_rank);
        row_len = msg[0];
        matrix_len = msg[1];
    }

    long chunk_size = matrix_len / (nmb_mpi_proc - 1);
    long offset = chunk_size * (mpi_rank - 1);

    for (long iter = 0; iter < iterations; iter++) {
        // printf("worker %d: receiving matrix\n", mpi_rank);
        double* matrix = (double*) malloc(sizeof(double) * matrix_len);
        error = MPI_Bcast(matrix, matrix_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
        assert_success(error, "worker receive matrix");
        // printf("worker %d: done\n", mpi_rank);

        double* result = (double*) malloc(sizeof(double) * chunk_size);
        for (long i = 0; i < chunk_size; i++) {
            double res = apply_heat_diffusion(matrix, i + offset, row_len, matrix_len, diffusion_constant);
            result[i] = res;
        }

        // printf("worker %d: sending result\n", mpi_rank);
        error = MPI_Send(result, chunk_size, MPI_DOUBLE, MASTER_RANK, RESULT_TAG, MPI_COMM_WORLD);
        assert_success(error, "worker send result");
        // printf("worker %d: done\n", mpi_rank);
    }
}

void read_input_file(char* filename, size_t* rows, size_t* cols, double** matrix) {
    FILE* f = fopen(filename, "r");
    char* line;
    int read = fscanf(f, "%zu %zu\n", cols, rows);
    if (read < 2) {
        printf("error read input file\n");
    }

    size_t matrix_len = (*rows) * (*cols);
    double* m = (double*) malloc(sizeof(double) * matrix_len);
    for (size_t i = 0; i < matrix_len; i++) {
        m[i] = 0;
    }

    size_t row, col;
    double val;
    while ((read = fscanf(f, "%zu %zu %lf\n", &col, &row, &val)) == 3) {
        m[row * (*cols) + col] = val;
    }

    *matrix = m;
}

double apply_heat_diffusion(double* matrix, long i, long row_len, long matrix_len, double diffusion_constant) {
    double sum = 0;
    sum += matrix[i-1]; 
    sum += matrix[i+1];
    sum += matrix[i-row_len]; 
    sum += matrix[i+row_len];
    
    // // left
    // if (i % row_len != 0) {
    //     sum += matrix[i-1];
    // }

    // // right
    // if (i % row_len != row_len - 1) {
    //     sum += matrix[i+1];
    // }

    // // up
    // if (i - row_len >= 0) {
    //     sum += matrix[i-row_len];
    // }

    // // down
    // if (i + row_len < matrix_len) {
    //     sum += matrix[i+row_len];
    // }

    return matrix[i] + diffusion_constant * (sum / 4 - matrix[i]);
}

void assert_success(int error, char* msg) {
    if (error != MPI_SUCCESS) {
        printf("error: %s\n", msg);
        exit(1);
    }
}

void print_matrix(double* matrix, long matrix_len) {
    for (long i = 0; i < matrix_len; i++) {
        printf("%d: %lf\n", i, matrix[i]);
    }
}

double average(double* matrix, size_t matrix_len) {
  double avg = 0;
  for (size_t i = 1; i <= matrix_len; i++) {
    double x = matrix[i-1];
    avg += (x - avg) / i;
  }
  return avg;
}

double average_diff(double* matrix, double avg, size_t matrix_len) {
  double avg_diff = 0;
  for (size_t i = 1; i <= matrix_len; i++) {
    double x = abs(matrix[i-1] - avg);
    avg_diff += (x - avg_diff) / i;
  }
  return avg_diff;
}
