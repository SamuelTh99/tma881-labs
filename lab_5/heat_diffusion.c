#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <mpi.h>

void main_master(int nmb_mpi_proc, char* filename, long iterations, double diffusion_constant);
void main_worker(int nmb_mpi_proc, int mpi_rank, long iterations, double diffusion_constant);
long read_input_file(char* filename, long* rows, long* cols, double** matrix);
long get_matrix_index(long row, long col, long row_len);
double apply_heat_diffusion(double* m, long i, long row_len, long n, double c);
void assert_success(int error, char* msg);
void print_matrix(double* matrix, long matrix_len, long row_len);
double average(double* matrix, long rows, long cols);
double average_diff(double* matrix, double avg, long rows, long cols);

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
    long matrix_len = read_input_file(filename, &rows, &cols, &matrix);
    long chunk_size = rows / (nmb_mpi_proc - 1);

    if (nmb_mpi_proc == 1) {
        double* m = (double*) malloc(sizeof(double) * matrix_len);
        for (long iter = 0; iter < iterations; iter++) {
            for (long r = 0; r < rows; r++) {
                for (long c = 0; c < cols; c++) {
                    long i = get_matrix_index(r, c, cols);
                    m[i] += apply_heat_diffusion(matrix, i, cols, matrix_len, diffusion_constant);
                }
            }
            double* tmp = m;
            m = matrix;
            matrix = tmp;
        }
    } else { 
        {
            int len = 3;
            long msg[len];
            msg[0] = rows;
            msg[1] = cols;
            msg[2] = matrix_len;
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
                int result_len = chunk_size * cols;
                double* result = (double*) malloc(sizeof(double) * result_len);
                error = MPI_Recv(result, result_len, MPI_DOUBLE, rank, RESULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                assert_success(error, "master recv result");
                // printf("master: done\n");

                long row_offset = chunk_size * (rank - 1);
                for (long r = 0; r < chunk_size; r++) {
                    for (long c = 0; c < cols; c++) {
                        long i = get_matrix_index(r + row_offset, c, cols);
                        matrix[i] = result[c + (r * cols)];
                    }
                    //long i = get_matrix_index(r + row_offset, 0, cols);
                    //memcpy(&matrix + i * sizeof(double), &result + r * cols * sizeof(double), sizeof(double) * cols);
                    // matrix[i + offset] = result[i];
                }
            }

            printf("iteration %d\n", iter);
            print_matrix(matrix, matrix_len, cols);
        }
    }

    // Calculate & print averages.
    double avg = average(matrix, rows,cols); 
    double avg_diff = average_diff(matrix, avg, rows, cols);
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
    long rows, cols, matrix_len;

    {
        int len = 3;
        long msg[len];
        // printf("worker %d: receiving dimensions\n", mpi_rank);
        error = MPI_Bcast(msg, len, MPI_LONG, MASTER_RANK, MPI_COMM_WORLD);
        assert_success(error, "worker receive dimensions");
        // printf("worker %d: done\n", mpi_rank);
        rows = msg[0];
        cols = msg[1];
        matrix_len = msg[2];
    }

    long chunk_size = rows / (nmb_mpi_proc - 1);
    long row_offset = chunk_size * (mpi_rank - 1);

    double* matrix = (double*) malloc(sizeof(double) * matrix_len); //TODO: detta borde gå att optimera? 
    for (long iter = 0; iter < iterations; iter++) {
        // printf("worker %d: receiving matrix\n", mpi_rank);
        error = MPI_Bcast(matrix, matrix_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
        assert_success(error, "worker receive matrix");
        // printf("worker %d: done\n", mpi_rank);

        int result_len = chunk_size * cols;
        double* result = (double*) malloc(sizeof(double) * result_len);
        long result_i = 0;
        for (long r = row_offset; r < row_offset + chunk_size; r++) {
            for (long c = 0; c < cols; c++) {
                long i = get_matrix_index(r, c, cols);
                result[result_i] = apply_heat_diffusion(matrix, i, cols, matrix_len, diffusion_constant);
                result_i++;
            }
        }

        // printf("worker %d: sending result\n", mpi_rank);
        error = MPI_Send(result, result_len, MPI_DOUBLE, MASTER_RANK, RESULT_TAG, MPI_COMM_WORLD);
        assert_success(error, "worker send result");
        // printf("worker %d: done\n", mpi_rank);
    }
}

long read_input_file(char* filename, long* rows, long* cols, double** matrix) {
    FILE* f = fopen(filename, "r");

    // Read dimensions.
    int read = fscanf(f, "%zu %zu\n", cols, rows);
    if (read < 2) {
        printf("error read input file\n");
    }

    // Allocate empty matrix.
    long r = (*rows), c = (*cols);
    long matrix_len = (r+2) * (c+2);
    double* m = (double*) malloc(sizeof(double) * matrix_len);
    for (long i = 0; i < matrix_len; i++) {
        m[i] = 0;
    }

    // Read matrix values.
    long row, col, i;
    double val;
    while ((read = fscanf(f, "%zu %zu %lf\n", &col, &row, &val)) == 3) {
        i = get_matrix_index(row, col, *cols);
        m[i] = val;
    }
    *matrix = m;

    return matrix_len;
}

long get_matrix_index(long row, long col, long row_len) {
    // Without padding:
    // return row * row_len + col
    return (row+1) * (row_len+2) + col + 1;
}

double apply_heat_diffusion(double* matrix, long i, long row_len, long matrix_len, double diffusion_constant) {
    double sum = 0;
    sum += matrix[i-1]; 
    sum += matrix[i+1];
    sum += matrix[i-row_len-2]; 
    sum += matrix[i+row_len+2];
    return diffusion_constant * (sum / 4 - matrix[i]);
}

void assert_success(int error, char* msg) {
    if (error != MPI_SUCCESS) {
        printf("error: %s\n", msg);
        exit(1);
    }
}

void print_matrix(double* matrix, long matrix_len, long row_len) {
    for (long i = 0; i < matrix_len; i++) {
        printf("%lf \t", i, matrix[i]);
        if (i % (row_len + 2) == (row_len + 2) - 1) {
            printf("\n");
        }
    }
}

double average(double* matrix, long rows, long cols) {
    double avg = 0;
    long d = 1;
    for (long r = 0; r < rows; r++) {
        for (long c = 0; c < cols; c++) {
            long i = get_matrix_index(r, c, cols);
            double x = matrix[i];
            avg += (x - avg) / d;
            d++;
        }
    }
    return avg;
}

double average_diff(double* matrix, double avg, long rows, long cols) {
    double avg_diff = 0;
    long d = 1;
    for (long r = 0; r < rows; r++) {
        for (long c = 0; c < cols; c++) {
            long i = get_matrix_index(r, c, cols);
            double x = abs(matrix[i] - avg);
            avg_diff += (x - avg_diff) / d;
            d++;
        }
    }
    return avg_diff;
}
