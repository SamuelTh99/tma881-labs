#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h> 
#include <omp.h> 

#define MAX_LINES 100000
#define LINE_LENGTH 24
#define MAX_DIST 3466
#define FILENAME "cells"

struct coord {
    short n1; 
    short n2;
    short n3;
};

void cell_distances(long dist_counts[], char* filename);
size_t read_chunk(struct coord chunk[], FILE* fp);
struct coord parse_coord(char* line);
short parse_pos(char* str);
void compute_distances_within_chunk(long dist_counts[], struct coord chunk[], size_t chunk_size);
void compute_distances_between_chunks(long dist_counts[], struct coord chunk_1[], size_t chunck_1_size, struct coord chunk_2[], size_t chunk_2_size);
short compute_distance(struct coord c1, struct coord c2);
void print_results(long dist_counts[]);

int main(int argc, char* argv[]) {
    if (argc < 2 || argv[1][0] != '-' || argv[1][1] != 't') {
        printf("Usage: ./cell_distances -t<num_threads>\n");
        exit(1);
    }

    char* filename = FILENAME;
    if (argc == 3) {
        filename = argv[2];
    }

    int num_threads = atoi(argv[1] + 2);
    omp_set_num_threads(num_threads);

    long dist_counts[MAX_DIST];
    cell_distances(dist_counts, filename);
    print_results(dist_counts);

    return 0;
}

void cell_distances(long dist_counts[], char* filename) {
    struct coord chunk_1[MAX_LINES];
    struct coord chunk_2[MAX_LINES];

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("could not open file %s\n", filename);
        exit(1);   
    }

    for (size_t i = 0; i < MAX_DIST; i++) {
        dist_counts[i] = 0;
    }

    size_t chunk_1_size;
    long i = 0;
    while ((chunk_1_size = read_chunk(chunk_1, fp)) > 0) {
        compute_distances_within_chunk(dist_counts, chunk_1, chunk_1_size);
        
        size_t chunk_2_size;
        while ((chunk_2_size = read_chunk(chunk_2, fp)) > 0) {
            compute_distances_between_chunks(dist_counts, chunk_1, chunk_1_size, chunk_2, chunk_2_size);
        }

        i++;

        fseek(fp, i * MAX_LINES * LINE_LENGTH, SEEK_SET);
    }

    fclose(fp);
}

size_t read_chunk(struct coord chunk[], FILE* fp) {
    char line[LINE_LENGTH];
    long lines_read = 0;
    while (lines_read < MAX_LINES && fread(line, sizeof(char), LINE_LENGTH, fp) == LINE_LENGTH) {
        chunk[lines_read] = parse_coord(line);
        lines_read++;
    }
    return lines_read;
}

struct coord parse_coord(char* line) {
    struct coord c;
    c.n1 = parse_pos(line);
    c.n2 = parse_pos(line + 8);
    c.n3 = parse_pos(line + 16);
    return c;
}

short parse_pos(char* str) {
    short n = (str[1] - 48) * 10000;
    n += (str[2] - 48) * 1000;
    n += (str[4] - 48) * 100;
    n += (str[5] - 48) * 10;
    n += (str[6] - 48);
    if (str[0] == '-') {
        n = -n;
    }
    return n;
}

void compute_distances_within_chunk(long dist_counts[], struct coord chunk[], size_t chunk_size) {
    #pragma omp parallel for reduction(+:dist_counts[:MAX_DIST])
    for (size_t i = 0; i < chunk_size - 1; i++) {
        for (size_t j = i + 1; j < chunk_size; j++) {
            struct coord c1 = chunk[i];
            struct coord c2 = chunk[j];
            short dist = compute_distance(c1, c2);
            dist_counts[dist]++;
        }
    }
}

void compute_distances_between_chunks(long dist_counts[], struct coord chunk_1[], size_t chunk_1_size, struct coord chunk_2[], size_t chunk_2_size) {
    #pragma omp parallel for collapse(2) reduction(+:dist_counts[:MAX_DIST])
    for (size_t i = 0; i < chunk_1_size; i++) {
        for (size_t j = 0; j < chunk_2_size; j++) {
            struct coord c1 = chunk_1[i];
            struct coord c2 = chunk_2[j];
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
    return (short) (d / 10.0);
}

void print_results(long dist_counts[]) {
    for (size_t i = 0; i < MAX_DIST; i++) {
        long count = dist_counts[i];
        if (count != 0) {
            printf("%05.2f %ld\n", ((double) i) / 100.0, count);
        }
    }
}
