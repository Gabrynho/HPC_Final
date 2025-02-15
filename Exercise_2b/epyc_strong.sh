#!/bin/bash

#SBATCH --job-name=epyc_pqrs
#SBATCH -A dssc
#SBATCH --partition=EPYC
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=128
#SBATCH --time=02:00:00
#SBATCH --output=epyc_pqrs_%j.out
#SBATCH --exclusive
#SBATCH --mail-type=ALL
#SBATCH  --mail-user=masellagabriel@gmail.com

module load openMPI/4.1.6/gnu/14.2.1

# Fixed global data size for strong scaling
DATA_SIZE=10000000

mpicc -o parallel_quicksort_strong parallel-quicksort-mpi/src/main.c parallel-quicksort-mpi/src/quicksort.c -lm
for np in $(seq 228 2 256); do
  for i in {1..10}; do
    if [ $i -eq 1 ]; then
      mpirun -np $np ./parallel_quicksort_strong ${DATA_SIZE} 1 timings_strong_scaling_two_nodes.csv
    else
      mpirun -np $np ./parallel_quicksort_strong ${DATA_SIZE} 0 timings_strong_scaling_two_nodes.csv
    fi
  done
done
