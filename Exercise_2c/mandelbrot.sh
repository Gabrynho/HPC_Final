#!/bin/bash
#SBATCH --job-name=mandelbrot
#SBATCH -A dssc
#SBATCH --partition=EPYC
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=128
#SBATCH --time=02:00:00
#SBATCH --output=mandelbrot_bench_%j.out
#SBATCH --exclusive
#SBATCH --mail-type=ALL
#SBATCH --mail-user=s269856@ds.units.it

# Load necessary modules
module load openMPI/4.1.6/gnu/14.2.1

# Compile the Mandelbrot program with timing support
gcc -fopenmp mandelbrot_openmp_timed.c pgm_util.c -o mandelbrot_exe

# Set the CSV output file and write the header line
CSV_OUT="mandelbrot_results.csv"
echo "Num_Threads,n_x,n_y,x_L,y_L,x_R,y_R,I_max,Calc_Time" > "${CSV_OUT}"

# Loop over different numbers of OpenMP threads for benchmarking
for IMAX in 255 5000 10000 30000 65565; do
  for OMP_THREADS in 1 2 4 8 16 32 64 128; do
    export OMP_NUM_THREADS=${OMP_THREADS}
    echo "Running with OMP_NUM_THREADS=${OMP_THREADS}"
      for i in {1..10}
      # Run the executable with the following arguments:
      # n_x n_y x_L y_L x_R y_R I_max
      # Example: 1024x768 pixels, complex plane region [-2.0,1.0]x[-1.0,1.0] and I_max=255.
      RESULT=$(./mandelbrot_exe 1024 768 -2.0 -1.0 1.0 1.0 ${IMAX})

      # Append the CSV output from the executable to CSV_OUT.
      echo "${RESULT}" >> "${CSV_OUT}"
      done
  done
done
