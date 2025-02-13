#!/bin/bash

#SBATCH --job-name=epyc_pqrs
#SBATCH -A dssc
#SBATCH --partition=EPYC
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=128  
#SBATCH --time=02:00:00
#SBATCH --output=epyc_pqrs_%j.out
#SBATCH --exclusive

module load openMPI/4.1.6/gnu/14.2.1

# Fixed global data size for strong scaling
DATA_SIZE=10000000

for np in {2,2,128}; do
    mpirun -np $np ./parallel_quicksort ${DATA_SIZE}
done

