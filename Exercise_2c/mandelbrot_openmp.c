#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

// Declaration of write_pgm_image from pgm_util.c
void write_pgm_image(void *image, int maxval, int xsize, int ysize, const char *image_name);

int main(int argc, char **argv) {
    if (argc != 8) {
        fprintf(stderr, "Usage: %s n_x n_y x_L y_L x_R y_R I_max\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Parse command-line arguments
    int n_x = atoi(argv[1]);
    int n_y = atoi(argv[2]);
    double x_L = atof(argv[3]);
    double y_L = atof(argv[4]);
    double x_R = atof(argv[5]);
    double y_R = atof(argv[6]);
    int I_max = atoi(argv[7]);

    // Determine pixel data type: use unsigned char if I_max <= 255, otherwise unsigned short int.
    int useShort = (I_max > 255) ? 1 : 0;
    int imageSize = n_x * n_y;
    int maxval = I_max;  // Maximum value for PGM header

    // Calculate increments in the complex plane
    double dx = (x_R - x_L) / n_x;
    double dy = (y_R - y_L) / n_y;

    // Allocate memory for image array
    void *image = NULL;
    if (!useShort) {
        image = malloc(imageSize * sizeof(unsigned char));
        if (image == NULL) {
            fprintf(stderr, "Error allocating image memory\n");
            return EXIT_FAILURE;
        }
    } else {
        image = malloc(imageSize * sizeof(unsigned short int));
        if (image == NULL) {
            fprintf(stderr, "Error allocating image memory\n");
            return EXIT_FAILURE;
        }
    }

    // Measure the start time of the Mandelbrot calculation
    double start_time = omp_get_wtime();

    // Compute the Mandelbrot set using OpenMP parallelism
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int j = 0; j < n_y; j++) {
        for (int i = 0; i < n_x; i++) {
            double c_re = x_L + i * dx;
            double c_im = y_L + j * dy;
            double z_re = 0.0, z_im = 0.0;
            int n;
            for (n = 1; n <= I_max; n++) {
                double z_re2 = z_re * z_re;
                double z_im2 = z_im * z_im;
                if (z_re2 + z_im2 > 4.0) {
                    break;
                }
                double new_re = z_re2 - z_im2 + c_re;
                double new_im = 2.0 * z_re * z_im + c_im;
                z_re = new_re;
                z_im = new_im;
            }
            // Set pixel value: 0 if point is in the set, or iteration count when it escaped
            int pixelValue = (n > I_max) ? 0 : n;
            int index = j * n_x + i;
            if (!useShort) {
                ((unsigned char*)image)[index] = (unsigned char)pixelValue;
            } else {
                ((unsigned short int*)image)[index] = (unsigned short int)pixelValue;
            }
        }
    }
    
    // Measure the end time of the Mandelbrot calculation
    double end_time = omp_get_wtime();
    double calc_time = end_time - start_time;
    
    // Output a CSV formatted line to stdout with:
    // Type, Num_Threads, n_x, n_y, Calculation_Time (in seconds)
    int num_threads = omp_get_max_threads();
    printf("OpenMP,%d,%d,%d,%.6f\n", num_threads, n_x, n_y, calc_time);

    // Optionally, write the image to a PGM file, so it can be viewed later if desired
    write_pgm_image(image, maxval, n_x, n_y, "mandelbrot.pgm");

    free(image);
    return EXIT_SUCCESS;
}