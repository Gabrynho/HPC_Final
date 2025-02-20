#ifndef QUICKSORT_H
#define QUICKSORT_H

#include <mpi.h>

void quicksort(double *array, int left, int right);
void parallelQuicksort(double *data, int size, MPI_Comm comm);

#endif