#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h> 

// Implementation plan:
// 1. Function for doing Newton's method for a single x value
// 2. Function for doing Newton's method for a 1000x1000 grid using a single thread, writing result to file
// 3. Multithreaded implementation of above
// 4. Parse command line args

#define OUT_OF_BOUNDS 10000000000
#define ERROR_MARGIN 0.001

double complex roots[5];
int num_roots;

void print_complex_double(double complex d);
int newton(double complex x, int d);
bool illegal_value(double complex x);
int get_nearby_root(double complex x);
double complex next_x(double complex x, int d);
double complex f(double complex x, int d);
double complex f_deriv(double complex x, int d);

int main() {
    roots[0] = 1;
    roots[1] = -cpow(-1, 0.2);
    roots[2] = cpow(-1, 0.4);
    roots[3] = -cpow(-1, 0.6);
    roots[4] = cpow(-1, 0.8);
    num_roots = 5;

    double complex x = 100 + 100*I;
    int d = 5;

    int root = newton(x, d);

    printf("%d\n", root);
    print_complex_double(roots[root]);

    return 0;
}

void print_complex_double(double complex d) {
    printf("%f%+fi\n", creal(d), cimag(d));
}

int newton(double complex x, int d) {
    for (int i = 0; ; i++) {
        if (illegal_value(x)) {
            break;
        }

        int root = get_nearby_root(x);
        if (root != -1) {
            return root;
        }

        x = next_x(x, d);
    }

    return -1;
}

// TODO: Flip order of checks, most common should be first
bool illegal_value(double complex x) {
    if (fabs(creal(x)) > OUT_OF_BOUNDS || fabs(cimag(x)) > OUT_OF_BOUNDS || cabs(x) < ERROR_MARGIN) { // out of bounds OR too close to origin
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

double complex next_x(double complex x, int d) {
    return x - f(x, d) / f_deriv(x, d);
}

double complex f(double complex x, int d) {
    return cpow(x, d) - 1;
}

double complex f_deriv(double complex x, int d) {
    return d * cpow(x, d - 1);
}
