#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <mpi.h>

void main_master(int nmb_mpi_proc, char* filename, long iterations, double diffusion_constant);
void main_worker(int nmb_mpi_proc, int mpi_rank, long iterations, double diffusion_constant);
void read_dimensions(FILE* f, long* rows, long* cols);
void read_matrix(FILE* f, double matrix[], long cols);
long get_matrix_index(long row, long col, long row_len);
void apply_heat_diffusion_on_rows(double* result, double* matrix, long row_offset, long chunk_size, long rows, long cols, long matrix_len, double diffusion_constant);
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
    read_matrix(f, matrix, cols);

    if (nmb_mpi_proc == 1) {
        double* m = (double*) malloc(sizeof(double) * matrix_len);
        double* tmp;
        for (long iter = 0; iter < iterations; iter++) {
            long i = cols + 3;
            for (long r = 0; r < rows; r++) {
                for (long c = 0; c < cols; c++) {
                    m[i] = apply_heat_diffusion(matrix, i, cols, matrix_len, diffusion_constant);
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

        long chunk_size = rows / nmb_mpi_proc;
        long master_row_offset = 0;
        int result_len = chunk_size * cols;
        double* result = (double*) malloc(sizeof(double) * result_len);

        // Start computations.
        for (long iter = 0; iter < iterations; iter++) {
            printf("iteration %ld\n", iter);

            error = MPI_Bcast(matrix, matrix_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
            assert_success(error, "master bcast");

            apply_heat_diffusion_on_rows(result, matrix, master_row_offset, chunk_size, rows, cols, matrix_len, diffusion_constant);
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
    }

    // Calculate & print averages.
    double avg = average(matrix, rows, cols); 
    double avg_diff = average_diff(matrix, avg, rows, cols);
    printf("average: %lf\naverage absolute difference: %lf\n", avg, avg_diff);

    free(matrix);
}

void main_worker(int nmb_mpi_proc, int mpi_rank, long iterations, double diffusion_constant) {
    int error;
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

    long chunk_size = rows / nmb_mpi_proc;
    long row_offset = chunk_size * mpi_rank;
    int result_len = chunk_size * cols;

    double* matrix = (double*) malloc(sizeof(double) * matrix_len);
    double* result = (double*) malloc(sizeof(double) * result_len);
    for (long iter = 0; iter < iterations; iter++) {
        error = MPI_Bcast(matrix, matrix_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
        assert_success(error, "worker receive matrix");
        apply_heat_diffusion_on_rows(result, matrix, row_offset, chunk_size, rows, cols, matrix_len, diffusion_constant);
        error = MPI_Send(result, result_len, MPI_DOUBLE, MASTER_RANK, RESULT_TAG, MPI_COMM_WORLD);
        assert_success(error, "worker send result");
    }

    free(matrix);
    free(result);
}

void read_dimensions(FILE* f, long* rows, long* cols) {
    int read = fscanf(f, "%zu %zu\n", cols, rows);
    if (read < 2) {
        printf("error read input file\n");
    }
}

void read_matrix(FILE* f, double matrix[], long cols) {
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

void apply_heat_diffusion_on_rows(double* result, double* matrix, long row_offset, long chunk_size, long rows, long cols, long matrix_len, double diffusion_constant) {
    long i = (row_offset+1) * (cols+2) + 1;
    long result_i = 0;
    for (long r = row_offset; r < row_offset + chunk_size; r++) {
        for (long c = 0; c < cols; c++) {
            result[result_i] = apply_heat_diffusion(matrix, i, cols, matrix_len, diffusion_constant);
            result_i++;
            i++;
        }
        i += 2;
    }
}

double apply_heat_diffusion(double* matrix, long i, long row_len, long matrix_len, double diffusion_constant) {
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
