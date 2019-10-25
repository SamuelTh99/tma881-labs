#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <mpi.h>

void main_master(int nmb_mpi_proc, char* filename, long iterations, double diffusion_constant);
void main_worker(int nmb_mpi_proc, int mpi_rank, long iterations, double diffusion_constant);
void read_dimensions(FILE* f, long* rows, long* cols);
void read_matrix_values(FILE* f, double matrix[], long cols);
long get_matrix_index(long row, long col, long row_len);
void apply_heat_diffusion_on_rows(double* result, double* local_matrix, long chunk_size, long cols, double diffusion_constant);
double apply_heat_diffusion(double* matrix, long i, long row_len, double diffusion_constant);
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
    int error;

    // Read dimensions from input file.
    FILE* f = fopen(filename, "r");
    long rows, cols;
    read_dimensions(f, &rows, &cols);

    // Read matrix values from input file.
    long matrix_len = (rows+2) * (cols+2);
    double* matrix = (double*) malloc(sizeof(double) * matrix_len);
    for (long i = 0; i < matrix_len; i++) {
        matrix[i] = 0;
    }
    read_matrix_values(f, matrix, cols);

    if (nmb_mpi_proc == 1) {
        double* m = (double*) malloc(sizeof(double) * matrix_len);
        double* tmp;
        for (long iter = 0; iter < iterations; iter++) {
            long i = cols + 3;
            for (long r = 0; r < rows; r++) {
                for (long c = 0; c < cols; c++) {
                    m[i] = apply_heat_diffusion(matrix, i, cols, diffusion_constant);
                    i++;
                }
                i += 2;
            }
            tmp = m;
            m = matrix;
            matrix = tmp;
        }
        free(m);
    } else { 
        // Broadcast matrix dimensions.
        {
            int len = 3;
            long msg[len];
            msg[0] = rows;
            msg[1] = cols;
            msg[2] = matrix_len;
            error = MPI_Bcast(msg, len, MPI_LONG, MASTER_RANK, MPI_COMM_WORLD);
            assert_success(error, "master bcast");
        }

        // Set up data structures.
        long chunk_size = rows / nmb_mpi_proc;
        int result_len = chunk_size * cols;
        int local_matrix_len = (chunk_size + 2) * (cols + 2);

        double* result = (double*) malloc(sizeof(double) * result_len);
        double* local_matrix = (double*) malloc(sizeof(double) * local_matrix_len);

        int lens[nmb_mpi_proc], poss[nmb_mpi_proc];
        for (long rank = 0; rank < nmb_mpi_proc; rank++) {
            lens[rank] = local_matrix_len;
            long row_offset = chunk_size * rank;
            poss[rank] = row_offset * (cols + 2);
        }

        // Start computations.
        for (long iter = 0; iter < iterations; iter++) {
            // error = MPI_Bcast(matrix, matrix_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
            // assert_success(error, "master bcast");
            error = MPI_Scatterv(matrix, lens, poss, MPI_DOUBLE, local_matrix, local_matrix_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
            assert_success(error, "master sent and recived scattered matrix");

            apply_heat_diffusion_on_rows(result, local_matrix, chunk_size, cols, diffusion_constant);
            for (long r = 0; r < chunk_size; r++) {
                for (long c = 0; c < cols; c++) {
                    long i = get_matrix_index(r, c, cols);
                    matrix[i] = result[c + (r * cols)];
                }
            }

            for (long rank = 1; rank < nmb_mpi_proc; rank++) {
                error = MPI_Recv(result, result_len, MPI_DOUBLE, rank, RESULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                assert_success(error, "master recv result");

                long row_offset = chunk_size * rank;
                for (long r = 0; r < chunk_size; r++) {
                    for (long c = 0; c < cols; c++) {
                        long i = get_matrix_index(r + row_offset, c, cols);
                        matrix[i] = result[c + (r * cols)];
                    }
                }
            }

            // printf("iteration %ld\n", iter);
            // print_matrix(matrix, matrix_len, cols);
        }

        free(result);
        free(local_matrix);
    }

    // Calculate & print averages.
    double avg = average(matrix, rows, cols); 
    double avg_diff = average_diff(matrix, avg, rows, cols);
    printf("average: %lf\naverage absolute difference: %lf\n", avg, avg_diff);

    free(matrix);
}

void main_worker(int nmb_mpi_proc, int mpi_rank, long iterations, double diffusion_constant) {
    int error;

    // Receive matrix dimensions.
    long rows, cols, matrix_len;
    {
        int len = 3;
        long msg[len];
        error = MPI_Bcast(msg, len, MPI_LONG, MASTER_RANK, MPI_COMM_WORLD);
        assert_success(error, "worker receive dimensions");
        rows = msg[0];
        cols = msg[1];
        matrix_len = msg[2];
    }

    // Set up data structures.
    long chunk_size = rows / nmb_mpi_proc;
    int result_len = chunk_size * cols;
    int local_matrix_len = (chunk_size + 2) * (cols + 2);

    double* local_matrix = (double*) malloc(sizeof(double) * local_matrix_len);
    double* result = (double*) malloc(sizeof(double) * result_len);

    // Start computations.
    for (long iter = 0; iter < iterations; iter++) {
        // Receive part of matrix.
        error = MPI_Scatterv(NULL, NULL, NULL, NULL, local_matrix, local_matrix_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
        assert_success(error, "worker receive scattered matrix");

        // Compute heat diffusion on assigned part of matrix.
        apply_heat_diffusion_on_rows(result, local_matrix, chunk_size, cols, diffusion_constant);

        // Send back results to master.
        error = MPI_Send(result, result_len, MPI_DOUBLE, MASTER_RANK, RESULT_TAG, MPI_COMM_WORLD);
        assert_success(error, "worker send result");
    }

    printf("worker %d done\n", mpi_rank);

    free(local_matrix);
    free(result);
}

void read_dimensions(FILE* f, long* rows, long* cols) {
    int read = fscanf(f, "%zu %zu\n", cols, rows);
    if (read < 2) {
        printf("error read input file\n");
    }
}

void read_matrix_values(FILE* f, double matrix[], long cols) {
    long row, col, i;
    double val;
    int read;
    while ((read = fscanf(f, "%zu %zu %lf\n", &col, &row, &val)) == 3) {
        i = get_matrix_index(row, col, cols);
        matrix[i] = val;
    }
}

long get_matrix_index(long row, long col, long row_len) {
    return (row+1) * (row_len+2) + col + 1;
}

void apply_heat_diffusion_on_rows(double* result, double* local_matrix, long chunk_size, long cols, double diffusion_constant) {
    for (long r = 0, matrix_i = cols + 3, result_i = 0; r < chunk_size; r++, matrix_i += 2) {
        for (long c = 0; c < cols; c++, matrix_i++, result_i++) {
            result[result_i] = apply_heat_diffusion(local_matrix, matrix_i, cols, diffusion_constant);
        }
    }
}

double apply_heat_diffusion(double* matrix, long i, long row_len, double diffusion_constant) {
    double self = matrix[i];
    double left = matrix[i-1];
    double right = matrix[i+1];
    double above = matrix[i-row_len-2];
    double below = matrix[i+row_len+2];
    return self + diffusion_constant * ((left + right + above + below) / 4 - self);
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
    long i = cols + 3;
    for (long r = 0; r < rows; r++) {
        for (long c = 0; c < cols; c++) {
            double x = matrix[i];
            avg += (x - avg) / d;
            d++;
            i++;
        }
        i += 2;
    }
    return avg;
}

double average_diff(double* matrix, double avg, long rows, long cols) {
    double avg_diff = 0;
    long d = 1;
    long i = cols + 3;
    for (long r = 0; r < rows; r++) {
        for (long c = 0; c < cols; c++) {
            double x = fabs(matrix[i] - avg);
            avg_diff += (x - avg_diff) / d;
            d++;
            i++;
        }
        i += 2;
    }
    return avg_diff;
}
