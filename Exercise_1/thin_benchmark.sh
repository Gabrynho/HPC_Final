#!/bin/bash
#SBATCH --job-name=thin_bench
#SBATCH -A dssc
#SBATCH --partition=THIN         
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=12  # Full THIN node (2×12 cores)
#SBATCH --time=02:00:00
#SBATCH --output=thin_bench_%j.out
#SBATCH --exclusive

module load openMPI/4.1.6/gnu/14.2.1

# Broadcast Benchmark
for np in {1..24}; do
  for ALG in 1 2 3 5; do  # 1: basic linear, 2: chain, 3: pipeline, 5: binary tree
    mpirun --map-by core \
      --mca coll_tuned_use_dynamic_rules true \
      --mca coll_tuned_bcast_algorithm "${ALG}" \
      ../osu-micro-benchmarks-7.5/c/mpi/collective/blocking/osu_bcast -i 1000 -x 100 \
      -n "${np}" -m 64
  done
done

# Reduce Benchmark
for np in {1..24}; do
  for ALG in 1 2 3 4; do  # 1: linear, 2: chain, 3: pipeline, 4: binary
    mpirun --map-by core \
      --mca coll_tuned_use_dynamic_rules true \
      --mca coll_tuned_reduce_algorithm "${ALG}" \
      ../osu-micro-benchmarks-7.5/c/mpi/collective/blocking/osu_allreduce -i 1000 -x 100 \
      -n "${np}" -m 64
  done
done