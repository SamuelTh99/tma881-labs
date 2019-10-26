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
void apply_heat_diffusion(double* result, double* local_matrix, long num_rows, long row_len, double diffusion_constant);
double calculate_new_temperature(double* matrix, long i, long row_len, double diffusion_constant);
void assert_success(int error, char* msg);
void print_matrix(int rank, double* matrix, long matrix_len, long row_len);
double average(double* matrix, long rows, long cols);
double average_diff(double* matrix, double avg, long rows, long cols);
void swap(double** m1, double** m2);

#define MASTER_RANK 0
#define FILENAME "diffusion"
#define ROW_TAG 1
#define RESULT_TAG 2

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

    double* matrix;
    long num_rows, row_len, matrix_len;


    if (mpi_rank == MASTER_RANK) {
        printf("reading matrix ... ");
        fflush(stdout);
        FILE* f = fopen(filename, "r");

        // Read dimensions from input file.
        read_dimensions(f, &num_rows, &row_len);

        // Read matrix values from input file.
        matrix_len = (num_rows+2) * (row_len+2);
        matrix = (double*) malloc(sizeof(double) * matrix_len);
        for (long i = 0; i < matrix_len; i++) {
            matrix[i] = 0;
        }
        read_matrix_values(f, matrix, row_len);
        printf("done\n");
    } 

    if (nmb_mpi_proc == 1) {
        // If only one node, do everything locally.
        double* matrix_copy = (double*) malloc(sizeof(double) * matrix_len);
        for (long iter = 0; iter < iterations; iter++) {
            apply_heat_diffusion(matrix_copy, matrix, num_rows, row_len, diffusion_constant);
            swap(&matrix_copy, &matrix);
        }
        free(matrix_copy);
    } else {
        // Distribute dimensions.
        if (mpi_rank == MASTER_RANK) {
            printf("distributing dimensions ... ");
            fflush(stdout);
        }
        {
            int len = 3;
            long msg[len];
            if (mpi_rank == MASTER_RANK) {
                msg[0] = num_rows;
                msg[1] = row_len;
                msg[2] = matrix_len;
            }
            error = MPI_Bcast(msg, len, MPI_LONG, MASTER_RANK, MPI_COMM_WORLD);
            assert_success(error, "broadcast dimensions");
            if (mpi_rank != MASTER_RANK) {
                num_rows = msg[0];
                row_len = msg[1];
                matrix_len = msg[2];
            }
        }
        if (mpi_rank == MASTER_RANK) {
            printf("done\n");
        }

        // Distribute matrix.
        if (mpi_rank == MASTER_RANK) {
            printf("distributing matrix ... ");
            fflush(stdout);
        }
        long rows_per_worker = num_rows / nmb_mpi_proc;
        long padded_row_len = row_len + 2;
        int local_matrix_len = (rows_per_worker + 2) * padded_row_len;
        double* local_matrix = (double*) malloc(sizeof(double) * local_matrix_len);
        int lens[nmb_mpi_proc], poss[nmb_mpi_proc];
        if (mpi_rank == MASTER_RANK) {
            for (long rank = 0; rank < nmb_mpi_proc; rank++) {
                lens[rank] = local_matrix_len;
                long row_offset = rows_per_worker * rank;
                poss[rank] = row_offset * padded_row_len;
            }
        }
        error = MPI_Scatterv(matrix, lens, poss, MPI_DOUBLE, local_matrix, local_matrix_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
        assert_success(error, "scatter matrix");
        if (mpi_rank == MASTER_RANK) {
            printf("done\n");
        }

        // Begin computation.
        if (mpi_rank == MASTER_RANK) {
            printf("computing ... ");
            fflush(stdout);
        }
        double* local_matrix_copy = (double*) malloc(sizeof(double) * local_matrix_len);
        for (long i = 0; i < local_matrix_len; i++) {
            local_matrix_copy[i] = 0;
        }
        for (long iter = 0; iter < iterations; iter++) {
            apply_heat_diffusion(local_matrix_copy, local_matrix, rows_per_worker, row_len, diffusion_constant);
            swap(&local_matrix_copy, &local_matrix);

            // Send then receive.
            if (mpi_rank != 0) {
                // Send up and receive from above.
                error = MPI_Sendrecv(
                    local_matrix + padded_row_len, padded_row_len, MPI_DOUBLE, mpi_rank - 1, ROW_TAG,
                    local_matrix, padded_row_len, MPI_DOUBLE, mpi_rank - 1, ROW_TAG,
                    MPI_COMM_WORLD, NULL);
                assert_success(error, "send and recive from and to above");
            }
            if (mpi_rank != nmb_mpi_proc - 1) {
                // Send down and receive from below.
                error = MPI_Sendrecv(
                    local_matrix + local_matrix_len - 2*padded_row_len, padded_row_len, MPI_DOUBLE, mpi_rank + 1, ROW_TAG,
                    local_matrix + local_matrix_len - padded_row_len, padded_row_len, MPI_DOUBLE, mpi_rank + 1, ROW_TAG,
                    MPI_COMM_WORLD, NULL);
                assert_success(error, "send down and receive from below");
            }
        }
        if (mpi_rank == MASTER_RANK) {
            printf("done\n");
        }

        // Gather results to master.
        if (mpi_rank == MASTER_RANK) {
            printf("gathering results ... ");
            fflush(stdout);
        }
        int result_len = rows_per_worker * padded_row_len;
        error = MPI_Gather(local_matrix + padded_row_len, result_len, MPI_DOUBLE, matrix + padded_row_len, result_len, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
        assert_success(error, "gather results");
        if (mpi_rank == MASTER_RANK) {
            printf("done\n");
        }

        free(local_matrix);
        free(local_matrix_copy);
    }

    if (mpi_rank == MASTER_RANK) {
        // Calculate and print averages.
        printf("calculating averages ... ");
        fflush(stdout);
        double avg = average(matrix, num_rows, row_len); 
        double avg_diff = average_diff(matrix, avg, num_rows, row_len);
        printf("done\n");
        printf("average: %lf\naverage absolute difference: %lf\n", avg, avg_diff);
    }

    // Release resources.
    error = MPI_Finalize();
    assert_success(error, "finalize");

    if (mpi_rank == MASTER_RANK) {
        free(matrix);
    }
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

void apply_heat_diffusion(double* dst_matrix, double* src_matrix, long num_rows, long row_len, double diffusion_constant) {
    long i = row_len + 3;
    for (long r = 0; r < num_rows; r++) {
        for (long c = 0; c < row_len; c++) {
            dst_matrix[i] = calculate_new_temperature(src_matrix, i, row_len, diffusion_constant);
            i++;
        }
        i += 2;
    }
}

double calculate_new_temperature(double* matrix, long i, long row_len, double diffusion_constant) {
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

void print_matrix(int rank, double* matrix, long matrix_len, long row_len) {
    for (long i = 0; i < matrix_len; i++) {
        if (i % row_len == 0) {
            printf("node %d: ", rank);
        }
        printf("%lf \t", i, matrix[i]);
        if (i % row_len == row_len - 1) {
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

void swap(double** m1, double** m2) {
    double* tmp;
    tmp = *m1;
    *m1 = *m2;
    *m2 = tmp;
}
