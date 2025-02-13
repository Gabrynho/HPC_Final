#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <unistd.h>
#include "quicksort.h"

#define DEFAULT_DATA_SIZE 100000000  // Default size if not provided

void generate_data(double *data, int size) {
    for (int i = 0; i < size; i++) {
        data[i] = (double)rand() / RAND_MAX;
    }
}

/* Compare two double arrays with a tolerance. Returns 1 if identical, 0 otherwise. */
int compare_arrays(double *a, double *b, int n) {
    const double tol = 1e-6;
    for (int i = 0; i < n; i++) {
        if (fabs(a[i] - b[i]) > tol) {
            printf("Mismatch at index %d: parallel=%f, serial=%f\n", i, a[i], b[i]);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    int rank, size;
    int data_size;  // Global data size
    int run_serial = 1; // Control flag: 1 to run serial version, 0 to skip.
    double *global_data = NULL;
    double *serial_data = NULL;  // Copy for serial sorting

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Process command line arguments on rank 0.
    if (rank == 0) {
        // First argument: DATA_SIZE.
        if (argc > 1) {
            data_size = atoi(argv[1]);
            if (data_size <= 0) {
                fprintf(stderr, "Invalid DATA_SIZE provided.\n");
                data_size = DEFAULT_DATA_SIZE;
            }
        } else {
            data_size = DEFAULT_DATA_SIZE;
        }
        // Second argument: run_serial flag (optional), default true (1).
        if (argc > 2) {
            run_serial = atoi(argv[2]);  // nonzero means true.
        }
        // Third argument: CSV filename (optional).
        char *csv_filename = "timings.csv";  // default filename
        if (rank == 0) {
            if (argc > 3) {
                csv_filename = argv[3];
            }
        }
    }
    // Broadcast data_size to all processes.
    MPI_Bcast(&data_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Rank 0 allocates and generates global data.
    if (rank == 0) {
        global_data = (double *)malloc(data_size * sizeof(double));
        if (!global_data) {
            fprintf(stderr, "Memory allocation failed for global_data.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        generate_data(global_data, data_size);
        // Only allocate serial_data if the serial version should run.
        if (run_serial) {
            serial_data = (double *)malloc(data_size * sizeof(double));
            if (!serial_data) {
                fprintf(stderr, "Memory allocation failed for serial_data.\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            memcpy(serial_data, global_data, data_size * sizeof(double));
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start_parallel = MPI_Wtime();

    /* Execute parallel quicksort (PSRS) */
    parallelQuicksort(global_data, data_size, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double parallel_time = MPI_Wtime() - start_parallel;

    if (rank == 0) {
        printf("Root: Final sorted data from parallel sort (first 10 elements):\n");
        for (int i = 0; i < (data_size < 10 ? data_size : 10); i++) {
            printf("%f ", global_data[i]);
        }
        printf("\n");
        printf("Parallel (PSRS) quicksort time: %f seconds\n", parallel_time);
    }

    if (rank == 0 && run_serial) {
        // Run serial quicksort, time it, and compare outcomes.
        double start_serial = MPI_Wtime();
        quicksort(serial_data, 0, data_size - 1);
        double serial_time = MPI_Wtime() - start_serial;
        printf("Serial quicksort time: %f seconds\n", serial_time);
        printf("Serial sorted data (first 10 elements):\n");
        for (int i = 0; i < (data_size < 10 ? data_size : 10); i++) {
            printf("%f ", serial_data[i]);
        }
        printf("\n");

        // Compare parallel and serial results.
        if (compare_arrays(global_data, serial_data, data_size)) {
            printf("Success: The parallel and serial sorted arrays are identical.\n");
        } else {
            printf("Error: The sorted arrays differ.\n");
        }
        free(serial_data);
    }

    // Write results to CSV file on rank 0.
    if (rank == 0) {
        const char *csv_filename = "timings.csv";
        FILE *csv;
        // Create file with header if it doesn't exist.
        if (access(csv_filename, F_OK) != 0) {
            csv = fopen(csv_filename, "w");
            if (!csv) {
                perror("Error creating CSV file");
            } else {
                fprintf(csv, "ntasks,global_data_size,parallel_time,serial_time,run_serial\n");
                fclose(csv);
            }
        }
        // Append results.
        csv = fopen(csv_filename, "a");
        if (!csv) {
            perror("Error opening CSV file");
        } else {
            double serial_time = (run_serial) ? 
                ((double)0) : 0;  // If serial is not run, CSV can have 0 or N/A.
            // In this case, if not run, nothing to compare so we print 0.
            fprintf(csv, "%d,%d,%f,%f,%d\n", size, data_size, parallel_time, (run_serial ? serial_time : 0), run_serial);
            fclose(csv);
        }
        free(global_data);
    }

    MPI_Finalize();
    return 0;
}
