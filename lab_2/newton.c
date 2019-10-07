#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h> 

// Implementation plan:
// x. Function for doing Newton's method for a single x value
// x. Function for doing Newton's method for a 1000x1000 grid using a single thread, writing result to file
// 3. Write results to file
// 4. Multithreaded implementation of above
// 5. Parse command line args

void print_complex_double(double complex dbl);
void find_roots();
struct result newton(double complex x);
bool illegal_value(double complex x);
int get_nearby_root(double complex x);
double complex next_x(double complex x);
double complex f(double complex x);
double complex f_deriv(double complex x);
void write_attractors_file();
void write_convergence_file();

#define OUT_OF_BOUNDS 10000000000
#define ERROR_MARGIN 0.001
#define X_MIN -2.0
#define X_MAX 2.0

char num_roots;
double complex* roots;

long picture_size;
char d;

struct result {
    char root;
    char iterations;
};

struct result* results_values;
struct result** results;

char colors[11][12] = {
    "105 105 105 ", //Gray: -1 (index = root_value + 1)
    "255 0   0   ",
    "100 80  182 ",
    "217 255 191 ",
    "57  88  115 ",
    "173 0   217 ",
    "76  0   0   ",
    "255 170 0   ",
    "0   217 58  ",
    "0   20  51  ",
    "87  26  102 "
};

int main() {
    d = 5;
    picture_size = 20;

    num_roots = d;
    roots = (double complex*) malloc(sizeof(double complex) * num_roots);
    roots[0] = 1;
    roots[1] = -cpow(-1, 0.2);
    roots[2] = cpow(-1, 0.4);
    roots[3] = -cpow(-1, 0.6);
    roots[4] = cpow(-1, 0.8);

    results_values = (struct result*) malloc(sizeof(struct result) * picture_size * picture_size);
    results = (struct result**) malloc(sizeof(struct result*) * picture_size);
    for (size_t i = 0, j = 0; i < picture_size; i++, j += picture_size) {
        results[i] = results_values + j;
    }


    find_roots();

    write_attractors_file();
/*
    for (size_t i = 0; i < picture_size; i++) {
        for (size_t j = 0; j < picture_size; j++) {
            printf("%d\t ", results[i][j].root);
        }
        printf("\n");
    }
    printf("\n");
    for (size_t i = 0; i < picture_size; i++) {
        for (size_t j = 0; j < picture_size; j++) {
            printf("%2d\t ", results[i][j].iterations);
        }
        printf("\n");
    }
*/

    free(results);
    free(results_values);
    return 0;
}

void print_complex_double(double complex dbl) {
    printf("%lf%+lfi\n", creal(dbl), cimag(dbl));
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

    for (int i = 0; ; i++) {
        if (illegal_value(x)) {
            res.root = -1;
            res.iterations = i;
            break;
        }

        int root = get_nearby_root(x);
        if (root != -1) {
            res.root = root;
            //res.iterations = min(i, 50);
            if (i > 50){
               res.iterations = 50;
            } else
            {
                res.iterations = i;
            }
            
            break;
        }

        x = next_x(x);
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

void write_attractors_file(){
    FILE * fp;
    fp = fopen("newton_attractors_file.ppm", "w");

    fprintf(fp, "P3\n%ld %ld\n255\n", picture_size, picture_size);

    char * buf = (char *) malloc(sizeof(char) * ((picture_size * 12) ));//+1));


    for (size_t i = 0; i < picture_size; i++) {
        for (size_t j = 0, buf_j = 0; j < picture_size; j++, buf_j+=12) {
            for (int x = 0; x < 12; x ++) {
                printf("%c",colors[results[i][j].root +1][x]);
                buf[buf_j+x] = colors[results[i][j].root +1][x];
            }
            //fwrite((colors[results[i][j].root +1]), sizeof(char), 12, fp);
        }
        //buf[(picture_size*12) +1] = "\n";
        printf("\n-----------\n");
        fprintf(fp, "\n");
        fwrite(buf, sizeof(char), picture_size * 12, fp);    
    }
}

void write_convergence_file(){
    // P3
    // L L
    // M   
}
