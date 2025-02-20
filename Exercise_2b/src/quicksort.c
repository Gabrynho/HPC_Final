#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <string.h>
#include "quicksort.h"

/* Forward declarations */
int partition(double *array, int left, int right);
void gather_samples(double *local_array, int local_size, double *samples, int num_procs);
void select_pivots(double *samples, int total_samples, double *pivots, int num_pivots);
void partition_data(double *local_array, int local_size, double *pivots, int num_pivots,
                    double **partitions, int *partition_sizes, int num_procs);
void merge_partitions(double *final_array, double **partitions, int *partition_sizes, int num_procs);

void parallelQuicksort(double *array, int size, MPI_Comm comm) {
    int rank, num_procs;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &num_procs);

    if (rank == 0)
        printf("PSRS: Running with %d processes.\n", num_procs);

    int local_size = size / num_procs;
    double *local_array = (double *)malloc(local_size * sizeof(double));
    if (!local_array) {
        fprintf(stderr, "Process %d: Error allocating local_array.\n", rank);
        MPI_Abort(comm, 1);
    }

    /* Scatter data from root to all processes */
    MPI_Scatter(array, local_size, MPI_DOUBLE,
                local_array, local_size, MPI_DOUBLE,
                0, comm);

    /* Step 1: Local quicksort */
    quicksort(local_array, 0, local_size - 1);
    printf("Process %d: Completed local quicksort.\n", rank);

    /* Step 2: Gather samples */
    double *samples = NULL;
    if (rank == 0)
        samples = (double *)malloc((num_procs - 1) * num_procs * sizeof(double));
    else
        samples = (double *)malloc((num_procs - 1) * sizeof(double));

    gather_samples(local_array, local_size, samples, num_procs);
    if(rank == 0)
        printf("Process %d: Gathered samples.\n", rank);

    /* Step 3: Select pivots */
    double *pivots = (double *)malloc((num_procs - 1) * sizeof(double));
    select_pivots(samples, (num_procs - 1) * num_procs, pivots, num_procs - 1);
    if(rank == 0) {
        printf("Process %d: Selected pivots: ", rank);
        for (int i = 0; i < num_procs - 1; i++) {
            printf("%f ", pivots[i]);
        }
        printf("\n");
    }
    free(samples);

    /* Step 4: Partition local data */
    double **partitions = (double **)malloc(num_procs * sizeof(double *));
    int *partition_sizes = (int *)malloc(num_procs * sizeof(int));
    partition_data(local_array, local_size, pivots, num_procs - 1,
                   partitions, partition_sizes, num_procs);
    free(local_array);
    free(pivots);


    /* Step 5: Exchange partitions with MPI_Alltoallv */

    /* 1. Exchange partition sizes */
    int *send_counts = partition_sizes; // already computed from partition_data
    int *send_displs = malloc(num_procs * sizeof(int));
    send_displs[0] = 0;
    for (int i = 1; i < num_procs; i++) {
        send_displs[i] = send_displs[i-1] + send_counts[i-1];
    }

    int *recv_counts = malloc(num_procs * sizeof(int));
    MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, comm);

    int *recv_displs = malloc(num_procs * sizeof(int));
    recv_displs[0] = 0;
    for (int i = 1; i < num_procs; i++) {
        recv_displs[i] = recv_displs[i-1] + recv_counts[i-1];
    }

    int total_recv = 0;
    for (int i = 0; i < num_procs; i++) {
        total_recv += recv_counts[i];
    }

    /* 2. Prepare send buffer.
    Concatenate the local partitions into one send_buffer.*/
    int total_send = 0;
    for (int i = 0; i < num_procs; i++) {
        total_send += send_counts[i];
    }
    double *send_buffer = malloc(total_send * sizeof(double));
    for (int i = 0; i < num_procs; i++) {
        if (send_counts[i] > 0) {
            memcpy(send_buffer + send_displs[i], partitions[i], send_counts[i] * sizeof(double));
        }
    }

    /* 3. Allocate receive buffer and call MPI_Alltoallv */
    double *recv_buffer = malloc(total_recv * sizeof(double));
    MPI_Alltoallv(send_buffer, send_counts, send_displs, MPI_DOUBLE,
                recv_buffer, recv_counts, recv_displs, MPI_DOUBLE,
                comm);

    printf("Process %d: Completed partition exchange.\n", rank);

    /* Step 6: Merge partitions
    Reorganize the received partitions pointers for merging.
    (The recv_counts array gives the size for each partition from different processes.
        In PSRS, process 'i' gathers all i-th partitions from all processes.)
    */
    double **recv_parts = malloc(num_procs * sizeof(double *));
    for (int i = 0; i < num_procs; i++) {
        recv_parts[i] = malloc(recv_counts[i] * sizeof(double));
        memcpy(recv_parts[i], recv_buffer + recv_displs[i], recv_counts[i] * sizeof(double));
    }

    /* Merge all received partitions */
    double *final_array = malloc(total_recv * sizeof(double));
    merge_partitions(final_array, recv_parts, recv_counts, num_procs);
    printf("Process %d: Merged final array, first 5 elements: ", rank);
    for (int i = 0; i < (total_recv < 5 ? total_recv : 5); i++) {
        printf("%f ", final_array[i]);
    }
    printf("\n");

    /* Step 7: Gather the final sorted arrays at root using MPI_Gatherv */
    int local_final = total_recv; // local final array size
    int *all_final_counts = NULL;
    int *gather_displs = NULL;
    if (rank == 0) {
        all_final_counts = malloc(num_procs * sizeof(int));
    }
    MPI_Gather(&local_final, 1, MPI_INT,
            all_final_counts, 1, MPI_INT, 0, comm);

    double *gather_buffer = NULL;
    if (rank == 0) {
        gather_displs = malloc(num_procs * sizeof(int));
        gather_displs[0] = 0;
        for (int i = 1; i < num_procs; i++) {
            gather_displs[i] = gather_displs[i - 1] + all_final_counts[i - 1];
        }
        /* 'array' (global_data) is allocated on rank 0 and is used as the gather buffer */
        gather_buffer = array;
    }
    MPI_Gatherv(final_array, local_final, MPI_DOUBLE,
                gather_buffer, all_final_counts, gather_displs, MPI_DOUBLE,
                0, comm);

    if (rank == 0) {
        free(all_final_counts);
        free(gather_displs);
    }

    /* Free allocated memory */
    free(send_buffer);
    free(recv_buffer);
    free(send_displs);
    free(recv_counts);
    free(recv_displs);
    for (int i = 0; i < num_procs; i++) {
        free(partitions[i]);
        free(recv_parts[i]);
    }
    free(partitions);
    free(recv_parts);
    free(partition_sizes);
    free(final_array);
}

void quicksort(double *array, int left, int right) {
    if (left < right) {
        int pivot_index = partition(array, left, right);
        quicksort(array, left, pivot_index - 1);
        quicksort(array, pivot_index + 1, right);
    }
}

int partition(double *array, int left, int right) {
    double pivot = array[right];
    int i = left - 1;
    for (int j = left; j < right; j++) {
        if (array[j] < pivot) {
            i++;
            double temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
    }
    double temp = array[i + 1];
    array[i + 1] = array[right];
    array[right] = temp;
    return i + 1;
}

static int compare_doubles(const void *a, const void *b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da < db) ? -1 : ((da > db) ? 1 : 0);
}

void gather_samples(double *local_array, int local_size, double *samples, int num_procs) {
    int k = num_procs - 1;
    int interval = local_size / (k + 1);
    double *local_samples = malloc(k * sizeof(double));
    if (!local_samples) {
        fprintf(stderr, "Error allocating local_samples.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (int i = 0; i < k; i++) {
        local_samples[i] = local_array[(i+1)*interval];
    }

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Gather(local_samples, k, MPI_DOUBLE,
               samples, k, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    free(local_samples);
}

void select_pivots(double *samples, int total_samples, double *pivots, int num_pivots) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        qsort(samples, total_samples, sizeof(double), compare_doubles);
        int group_size = total_samples / num_pivots;
        for (int i = 0; i < num_pivots; i++) {
            int idx = (i+1) * group_size;
            if (idx >= total_samples)
                idx = total_samples - 1;
            pivots[i] = samples[idx];
        }
    }
    MPI_Bcast(pivots, num_pivots, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}

void partition_data(double *local_array, int local_size, double *pivots, int num_pivots,
                    double **partitions, int *partition_sizes, int num_procs) {
    int part = 0, start = 0;
    for (part = 0; part < num_pivots; part++) {
        int count = 0;
        while (start < local_size && local_array[start] <= pivots[part]) {
            count++;
            start++;
        }
        partition_sizes[part] = count;
        partitions[part] = malloc(count * sizeof(double));
        if (partitions[part] && count > 0) {
            for (int i = 0; i < count; i++) {
                partitions[part][i] = local_array[start - count + i];
            }
        }
    }
    int count = local_size - start;
    partition_sizes[num_procs - 1] = count;
    partitions[num_procs - 1] = malloc(count * sizeof(double));
    if (partitions[num_procs - 1] && count > 0) {
        for (int i = 0; i < count; i++) {
            partitions[num_procs - 1][i] = local_array[start + i];
        }
    }
}

void merge_partitions(double *final_array, double **partitions, int *partition_sizes, int num_procs) {
    int total = 0;
    for (int i = 0; i < num_procs; i++) {
        total += partition_sizes[i];
    }
    int *indices = calloc(num_procs, sizeof(int));
    if (!indices) {
        fprintf(stderr, "Error allocating merge indices.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (int k = 0; k < total; k++) {
        int min_idx = -1;
        double min_val = 0.0;
        for (int i = 0; i < num_procs; i++) {
            if (indices[i] < partition_sizes[i]) {
                double curr = partitions[i][indices[i]];
                if (min_idx == -1 || curr < min_val) {
                    min_val = curr;
                    min_idx = i;
                }
            }
        }
        assert(min_idx != -1);
        final_array[k] = min_val;
        indices[min_idx]++;
    }
    free(indices);
}