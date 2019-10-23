#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <mpi.h>

void main_master(int nmb_mpi_proc);
void main_worker(int nmp_mpi_proc, int mpi_rank);

int main(int argc, char* argv[]) {
    // Init MPI.
    MPI_Init(&argc, &argv);

    int nmb_mpi_proc, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nmb_mpi_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if (mpi_rank == 0) {
        main_master(nmb_mpi_proc);
    } else if (mpi_rank == 1) {
        main_worker(nmb_mpi_proc, mpi_rank);
    }

    // Release resources.
    MPI_Finalize();
}

void main_master(int nmb_mpi_proc) {
    printf( "Number of processes: %d\n", nmb_mpi_proc );
    {
        int msg = 1; int len = 1; int dest_rank = 1; int tag = 1;
        MPI_Send(&msg, len, MPI_INT, dest_rank, tag, MPI_COMM_WORLD);
        printf( "MPI message sent from 0: %d\n", msg );
    }

    {
        int msg; int max_len = 1; int src_rank = 1; int tag = 1;
        MPI_Status status;
        MPI_Recv(&msg, max_len, MPI_INT, src_rank, tag, MPI_COMM_WORLD, &status);
        printf( "MPI message received at 0: %d\n", msg );
    }
}

void main_worker(int nmp_mpi_proc, int mpi_rank) {
    int msg;

    {
        int max_len = 1; int src_rank = 0; int tag = 1;
        MPI_Status status;
        MPI_Recv(&msg, max_len, MPI_INT, src_rank, tag, MPI_COMM_WORLD, &status);
        printf( "MPI message received at %d: %d\n", mpi_rank, msg );
    }

    ++msg;

    {
        int len = 1; int dest_rank = 0; int tag = 1;
        MPI_Send(&msg, len, MPI_INT, dest_rank, tag, MPI_COMM_WORLD);
        printf( "MPI message sent from %d: %d\n", mpi_rank, msg );
    }
}
