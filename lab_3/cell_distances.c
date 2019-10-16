#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h> 
#include <omp.h> 

#define CHUNK_SIZE 100000
#define NUM_DISTS 3464
#define FILENAME "cells"

struct coord {
    short n1; 
    short n2;
    short n3;
};

long read_file(char* lines[]);
void parse_coords(struct coord coords[], long num_coords, char* lines[]);
void compute_distances(long dist_counts[], long num_coords, struct coord coords[]);
short compute_distance(struct coord c1, struct coord c2);
void print_results(long dist_counts[]);

int main() {
    omp_set_num_threads(4);

    char* lines[CHUNK_SIZE];
    long num_coords = read_file(lines);

    struct coord coords[num_coords];
    parse_coords(coords, num_coords, lines);

    long dist_counts[NUM_DISTS];
    for (size_t dist = 0; dist < NUM_DISTS; dist++) {
        dist_counts[dist] = 0;
    }

    compute_distances(dist_counts, num_coords, coords);

    print_results(dist_counts);

    return 0;
}

long read_file(char* lines[]) {
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    FILE* fp = fopen(FILENAME, "r");
    if (fp == NULL) {
        printf("could not open file\n");
        exit(1);
    }

    int num_lines = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        lines[num_lines] = (char*) malloc(sizeof(char) * len);
        strcpy(lines[num_lines], line);
        num_lines++;
    }

    if (line) {
        free(line);
    }
    fclose(fp);

    return num_lines;
}

void parse_coords(struct coord coords[], long num_coords, char* lines[]) {
    #pragma omp parallel for shared(coords, num_coords, lines)
    for (size_t i = 0; i < num_coords; i++) {
        char* line = lines[i];

        short n1, n2, n3;

        n1 = (line[1] - 48) * 10000;
        n1 += (line[2] - 48) * 1000;
        n1 += (line[4] - 48) * 100;
        n1 += (line[5] - 48) * 10;
        n1 += (line[6] - 48);
        if (line[0] == '-') {
            n1 = -n1;
        }

        n2 = (line[9] - 48) * 10000;
        n2 += (line[10] - 48) * 1000;
        n2 += (line[12] - 48) * 100;
        n2 += (line[13] - 48) * 10;
        n2 += (line[14] - 48);
        if (line[8] == '-') {
            n2 = -n2;
        }
        
        n3 = (line[17] - 48) * 10000;
        n3 += (line[18] - 48) * 1000;
        n3 += (line[20] - 48) * 100;
        n3 += (line[21] - 48) * 10;
        n3 += (line[22] - 48);
        if (line[16] == '-') {
            n3 = -n3;
        }

        free(line);

        struct coord c;
        c.n1 = n1;
        c.n2 = n2;
        c.n3 = n3;
        coords[i] = c;
    }
}

void compute_distances(long dist_counts[], long num_coords, struct coord coords[]) {
    for (size_t i = 0; i < num_coords - 1; i++) {
        for (size_t j = i + 1; j < num_coords; j++) {
            struct coord c1 = coords[i];
            struct coord c2 = coords[j];
            short dist = compute_distance(c1, c2);
            dist_counts[dist]++;
        }
    }
}

short compute_distance(struct coord c1, struct coord c2){
    int d1 = c1.n1 - c2.n1;
    int d2 = c1.n2 - c2.n2;
    int d3 = c1.n3 - c2.n3;
    double d = sqrt(d1 * d1 + d2 * d2 + d3 * d3);
    return (short) lround(d / 10.0);
}

void print_results(long dist_counts[]) {
    for (size_t i = 0; i < NUM_DISTS; i++) {
        long count = dist_counts[i];
        if (count != 0) {
            printf("%.2f: %ld\n", ((double) i) / 100.0, count);
        }
    }
}
