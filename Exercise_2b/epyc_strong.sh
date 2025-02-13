#!/bin/bash

#SBATCH --job-name=epyc_pqrs
#SBATCH -A dssc
#SBATCH --partition=EPYC
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=128 
#SBATCH --time=02:00:00
#SBATCH --output=epyc_pqrs_%j.out
#SBATCH --exclusive

module load openMPI/4.1.6/gnu/14.2.1

# Fixed global data size for strong scaling
DATA_SIZE=10000000

mpicc -o parallel_quicksort parallel-quicksort-mpi/src/main.c parallel-quicksort-mpi/src/quicksort.c -lm
for np in $(seq 2 2 256); do
  for i in {1..10}; do
    mpirun -np $np ./parallel_quicksort ${DATA_SIZE}
  done
done

