#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h> 

// Implementation plan:
// x. Function for doing Newton's method for a single x value
// x. Function for doing Newton's method for a 1000x1000 grid using a single thread, writing result to file
// x. Write results to file
// 4. Multithreaded implementation of above
// 5. Parse command line args

void print_complex_double(double complex dbl);
void init_roots();
void init_results_matrix();
void find_roots();
struct result newton(double complex x);
bool illegal_value(double complex x);
int get_nearby_root(double complex x);
double complex next_x(double complex x);
double complex f(double complex x);
double complex f_deriv(double complex x);
void write_files();

#define OUT_OF_BOUNDS 10000000000
#define ERROR_MARGIN 0.001
#define X_MIN -2.0
#define X_MAX 2.0
#define MAX_ITERATIONS 50
#define COLOR_TRIPLET_LEN 12
#define GRAYSCALE_COLOR_LEN 4

long picture_size;
char d;

char num_roots;
double complex* roots;

struct result {
    char root;
    char iterations;
};

struct result* results_values;
struct result** results;

char attractors_colors[10][COLOR_TRIPLET_LEN] = {
    "105 105 105 ", // Color used for points that don't converge
    "255 0   0   ",
    "100 80  182 ",
    "217 255 191 ",
    "57  88  115 ",
    "173 0   217 ",
    "76  0   0   ",
    "255 170 0   ",
    "0   217 58  ",
    "0   20  51  "
};

int main() {
    d = 5;
    picture_size = 1000;

    init_roots();
    init_results_matrix();

    find_roots();

    write_files();

    free(roots);
    free(results);
    free(results_values);

    return 0;
}


void print_complex_double(double complex dbl) {
    printf("%lf%+lfi\n", creal(dbl), cimag(dbl));
}

void init_roots() {
    num_roots = d;
    roots = (double complex*) malloc(sizeof(double complex) * num_roots);
    roots[0] = 1;
    roots[1] = -cpow(-1, 0.2);
    roots[2] = cpow(-1, 0.4);
    roots[3] = -cpow(-1, 0.6);
    roots[4] = cpow(-1, 0.8);
}

void init_results_matrix() {
    results_values = (struct result*) malloc(sizeof(struct result) * picture_size * picture_size);
    results = (struct result**) malloc(sizeof(struct result*) * picture_size);
    for (size_t i = 0, j = 0; i < picture_size; i++, j += picture_size) {
        results[i] = results_values + j;
    }
}

void find_roots() {
    double step_size = fabs(X_MAX - X_MIN) / picture_size;
    double im = X_MIN;
    for (int i = 0; i < picture_size; i++, im += step_size) {
        double re = X_MIN;
        for (int j = 0; j < picture_size; j++, re += step_size) {
            double complex x = re + im * I;
            // TODO: Send in a pointer to a result struct instead of creating struct inside newton function?
            results[i][j] = newton(x);
        }
    }
}

struct result newton(double complex x) {
    struct result res;

    int i;
    for (i = 0; ; i++) {
        if (illegal_value(x)) {
            res.root = -1;
            break;
        }

        int root = get_nearby_root(x);
        if (root != -1) {
            res.root = root;
            break;
        }

        x = next_x(x);
    }

    if (i > MAX_ITERATIONS) {
        res.iterations = MAX_ITERATIONS;
    } else {
        res.iterations = i;
    }
    return res;
}

bool illegal_value(double complex x) {
    // TODO: Flip order of checks, most common should be first
    if (cabs(x) < ERROR_MARGIN || fabs(creal(x)) > OUT_OF_BOUNDS || fabs(cimag(x)) > OUT_OF_BOUNDS) {
        return true;
    }
    return false;
}

int get_nearby_root(double complex x) {
    for (char i = 0; i < num_roots; i++) {
        double complex root = roots[i];
        double dist = cabs(x - root); // NOTE: +, -, * and / are overloaded for complex numbers
        if (dist < ERROR_MARGIN) {
            return i;
        }
    }
    return -1;
}

double complex next_x(double complex x) {
    return x - f(x) / f_deriv(x);
}

double complex f(double complex x) {
    return cpow(x, d) - 1;
}

double complex f_deriv(double complex x) {
    return d * cpow(x, d - 1);
}

void write_files() {
    char attractors_filename[25];
    char convergence_filename[26];
    sprintf(attractors_filename, "newton_attractors_x%d.ppm", d);
    sprintf(convergence_filename, "newton_convergence_x%d.ppm", d);
    FILE* fp_attractors = fopen(attractors_filename, "w");
    FILE* fp_convergence = fopen(convergence_filename, "w");

    fprintf(fp_attractors, "P3\n%ld %ld\n255\n", picture_size, picture_size);
    fprintf(fp_convergence, "P2\n%ld %ld\n%d\n", picture_size, picture_size, MAX_ITERATIONS);

    // + 1 is to make space for newline character at end of line
    size_t buf_attractors_len = picture_size * COLOR_TRIPLET_LEN + 1;
    size_t buf_convergence_len = picture_size * GRAYSCALE_COLOR_LEN + 1;

    for (size_t i = 0; i < picture_size; i++) {
        char buf_attractors[buf_attractors_len];
        char buf_convergence[buf_convergence_len];

        for (size_t j = 0, offset_attractors = 0, offset_convergence = 0; j < picture_size; j++) {
            struct result result = results[i][j];
            
            char* root_color = attractors_colors[result.root + 1];
            strncpy(buf_attractors + offset_attractors, root_color, COLOR_TRIPLET_LEN);
            offset_attractors += COLOR_TRIPLET_LEN;

            sprintf(buf_convergence + offset_convergence, "%3d ", result.iterations);
            offset_convergence += GRAYSCALE_COLOR_LEN;
        }
        
        buf_attractors[buf_attractors_len - 1] = '\n';
        buf_convergence[buf_convergence_len - 1] = '\n';
        
        fwrite(buf_attractors, sizeof(char), buf_attractors_len, fp_attractors);
        fwrite(buf_convergence, sizeof(char), buf_convergence_len, fp_convergence);
    }

    fclose(fp_attractors);
    fclose(fp_convergence);
}
