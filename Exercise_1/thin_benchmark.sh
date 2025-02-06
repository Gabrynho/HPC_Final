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

# Set the CSV output file and write header line
CSV_OUT="results.csv"
echo "Type,Algorithm,NP,Size,Avg_Latency,Min_Latency,Max_Latency,Iterations" > "${CSV_OUT}"

# Broadcast Benchmark
for NP in {1..24}; do
  for ALG in 1 2 3 5; do  # 1: basic linear, 2: chain, 3: pipeline, 5: binary tree
    echo "Running Broadcast: NP=${NP}, ALG=${ALG}"
    result=$(mpirun --map-by core -n "${NP}" \
      --mca coll_tuned_use_dynamic_rules true \
      --mca coll_tuned_bcast_algorithm "${ALG}" \
      osu-micro-benchmarks-7.5/c/mpi/collective/blocking/osu_bcast -i 1000 -x 100 \
       -m 64:64 -f)

    # Append lines that start with a number (data rows) to CSV file, prefixing with type, algorithm, NP
    echo "$result" | awk -v type="Broadcast" -v alg="${ALG}" -v np="${NP}" '$1 ~ /^[0-9]+$/ {print type","alg","np","$1","$2","$3","$4","$5}' >> "${CSV_OUT}"
  done
done

# Reduce Benchmark
for NP in {1..24}; do
  for ALG in 1 2 3 4; do  # 1: linear, 2: chain, 3: pipeline, 4: binary
    echo "Running Reduce: NP=${NP}, ALG=${ALG}"
    result=$(mpirun --map-by core -n "${NP}" \
      --mca coll_tuned_use_dynamic_rules true \
      --mca coll_tuned_reduce_algorithm "${ALG}" \
      osu-micro-benchmarks-7.5/c/mpi/collective/blocking/osu_allreduce -i 1000 -x 100 \
      -m 64:64 -f)

    echo "$result" | awk -v type="Reduce" -v alg="${ALG}" -v np="${NP}" '$1 ~ /^[0-9]+$/ {print type","alg","np","$1","$2","$3","$4","$5}' >> "${CSV_OUT}"
  done
done