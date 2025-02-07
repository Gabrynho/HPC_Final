#!/bin/bash
#SBATCH --job-name=epyc_bench
#SBATCH -A dssc
#SBATCH --partition=EPYC
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=128  # Full EPYC node (2×128 cores)
#SBATCH --time=02:00:00
#SBATCH --output=epyc_bench_%j.out
#SBATCH --exclusive

module load openMPI/4.1.6/gnu/14.2.1

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 {broadcast|reduce}"
  exit 1
fi

OPERATION=$1

# Set the CSV output file and write header line
CSV_OUT="HPC_Final/Exercise_1/epyc_core_result.csv"
echo "Type,Algorithm,NP,Size,Avg_Latency,Min_Latency,Max_Latency,Iterations" > "${CSV_OUT}"

if [ "$OPERATION" == "broadcast" ]; then
  echo "Running Broadcast tests..."
  # Broadcast Operation: NP from 2 to 256, stepping by 2
  for NP in $(seq 2 2 256); do
    for ALG in 1 2 3 5; do  # 1: basic linear, 2: chain, 3: pipeline, 5: binary tree
      echo "Running Broadcast: NP=${NP}, ALG=${ALG}"
      result=$(mpirun --map-by core -n "${NP}" \
        --mca coll_tuned_use_dynamic_rules true \
        --mca coll_tuned_bcast_algorithm "${ALG}" \
        osu-micro-benchmarks-7.5/c/mpi/collective/blocking/osu_bcast -i 1000 -x 100 -f)
      
      echo "$result" | awk -v type="Broadcast" -v alg="${ALG}" -v np="${NP}" '$1 ~ /^[0-9]+$/ {print type","alg","np","$1","$2","$3","$4","$5}' >> "${CSV_OUT}"
    done
  done

elif [ "$OPERATION" == "reduce" ]; then
  echo "Running Reduce tests..."
  # Reduce Operation: NP from 2 to 256, stepping by 2
  for NP in $(seq 2 2 256); do
    for ALG in 1 2 3 4; do  # 1: linear, 2: chain, 3: pipeline, 4: binary
      echo "Running Reduce: NP=${NP}, ALG=${ALG}"
      result=$(mpirun --map-by core -n "${NP}" \
        --mca coll_tuned_use_dynamic_rules true \
        --mca coll_tuned_reduce_algorithm "${ALG}" \
        osu-micro-benchmarks-7.5/c/mpi/collective/blocking/osu_allreduce -i 1000 -x 100 -f)
      
      echo "$result" | awk -v type="Reduce" -v alg="${ALG}" -v np="${NP}" '$1 ~ /^[0-9]+$/ {print type","alg","np","$1","$2","$3","$4","$5}' >> "${CSV_OUT}"
    done
  done

else
  echo "Invalid operation. Use 'broadcast' or 'reduce'."
  exit 1
fi
