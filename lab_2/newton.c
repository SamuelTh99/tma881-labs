#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h> 

// Implementation plan:
// x. Function for doing Newton's method for a single x value
// 2. Function for doing Newton's method for a 1000x1000 grid using a single thread, writing result to file
// 3. Multithreaded implementation of above
// 4. Parse command line args

void print_complex_double(double complex dbl);
void find_roots();
int newton(double complex x);
bool illegal_value(double complex x);
int get_nearby_root(double complex x);
double complex next_x(double complex x);
double complex f(double complex x);
double complex f_deriv(double complex x);

#define OUT_OF_BOUNDS 10000000000
#define ERROR_MARGIN 0.001
#define X_MIN -2
#define X_MAX 2

int num_roots;
double complex* roots;

int picture_size;
int d;

int* results_values;
int** results;

int main() {
    d = 5;
    int l = 10;

    num_roots = d;
    roots = (double complex*) malloc(sizeof(double complex) * num_roots);
    roots[0] = 1;
    roots[1] = -cpow(-1, 0.2);
    roots[2] = cpow(-1, 0.4);
    roots[3] = -cpow(-1, 0.6);
    roots[4] = cpow(-1, 0.8);

    picture_size = l;
    
    results_values = (int*) malloc(l * l * sizeof(int));
    results = (int**) malloc(l * sizeof(int*));
    for (size_t i = 0, j = 0; i < l; i++, j += l) {
        results[i] = results_values + j;
    }

    find_roots();

    for (int i = 0; i < l; i++) {
        for (int j = 0; j < l; j++) {
            printf("%d ", results[i][j]);
        }
        printf("\n");
    }

    return 0;
}

void print_complex_double(double complex dbl) {
    printf("%lf%+lfi\n", creal(dbl), cimag(dbl));
}

void find_roots() {
    double step_size = (X_MAX - X_MIN) / ((double) picture_size);
    int i = 0, j = 0;
    for (double re = X_MIN; re <= X_MAX; re += step_size, i++) {
        for (double im = X_MIN; im <= X_MAX; im += step_size, j++) {
            printf("find_roots - j: %d", j);
            double complex x = re + im * I;
            int root = newton(x);
            results[i][j] = root;
        }
    }
}

int newton(double complex x) {
    for (int i = 0; ; i++) {
        if (illegal_value(x)) {
            break;
        }

        int root = get_nearby_root(x);
        if (root != -1) {
            return root;
        }

        x = next_x(x);
    }

    return -1;
}

// TODO: Flip order of checks, most common should be first
bool illegal_value(double complex x) {
    if (cabs(x) < ERROR_MARGIN || fabs(creal(x)) > OUT_OF_BOUNDS || fabs(cimag(x)) > OUT_OF_BOUNDS) { // out of bounds OR too close to origin
        return true;
    }
    return false;
}

int get_nearby_root(double complex x) {
    for (int i = 0; i < num_roots; i++) {
        double complex root = roots[i];
        double dist = cabs(x - root); // note: +, -, * and / are overloaded for complex numbers
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
