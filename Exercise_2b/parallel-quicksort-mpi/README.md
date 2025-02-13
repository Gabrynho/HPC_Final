# Parallel Quicksort MPI

This project implements a parallel version of the Quicksort algorithm using the Message Passing Interface (MPI). The implementation is designed to efficiently sort large datasets that may not fit into the memory of a single node by distributing the workload across multiple processes.

## Project Structure

The project consists of the following files:

- **src/main.c**: The entry point for the MPI application. This file initializes the MPI environment, manages data distribution among processes, and coordinates the execution of the parallel quicksort algorithm.

- **src/quicksort.c**: Contains the implementation of the parallel quicksort algorithm. It includes functions for performing local quicksort on each process's data segment, gathering local samples, selecting pivot values, partitioning data, and merging the sorted partitions.

- **src/quicksort.h**: The header file that declares the functions and data structures used in `quicksort.c`. It includes function prototypes for the parallel quicksort operations and any necessary data structures for handling the sorting process.

This will compile the source files and create the executable for the parallel quicksort application.

## Running the Application

To run the application, use the `mpirun` command followed by the number of processes and the name of the executable. For example:

```bash
mpirun -np <number_of_processes> ./parallel-quicksort <data_dimension>
```

Replace `<number_of_processes>` with the desired number of MPI processes.

## Notes

- The implementation assumes that the data to be sorted is uniformly distributed in the range `[0, 1)`.
- The data entries are of double-precision floating point type, encapsulated in a structure for sorting purposes.
- Ensure that the MPI library is properly installed and configured on your system before building and running the application.

## Acknowledgments

This project is part of an assignment to explore parallel computing concepts and the implementation of sorting algorithms in a distributed memory environment.
