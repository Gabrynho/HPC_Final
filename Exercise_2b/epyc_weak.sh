#!/bin/bash

#SBATCH --job-name=epyc_pqrs
#SBATCH -A dssc
#SBATCH --partition=EPYC
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=128
#SBATCH --time=02:00:00
#SBATCH --output=epyc_pqrs%j.out
#SBATCH --exclusive
#SBATCH --mail-type=ALL
#SBATCH --mail-user=masellagabriel@gmail.com

module load openMPI/4.1.6/gnu/14.2.1

# Set the data size per process for weak scaling.
DATA_SIZE_PER_PROC=1000000

# Compile the executable.
mpicc -o parallel_quicksort_weak parallel-quicksort-mpi/src/main.c parallel-quicksort-mpi/src/quicksort.c -lm

# Loop over different process counts.
for np in $(seq 96 2 128); do
  # For weak scaling, global data size = np * DATA_SIZE_PER_PROC.
  GLOBAL_DATA_SIZE=$(( np * DATA_SIZE_PER_PROC ))
  for i in {1..10}; do
    mpirun -np $np ./parallel_quicksort_weak ${GLOBAL_DATA_SIZE} 1 timings_weak_scaling.csv
  done
done
