#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h> 
#include <omp.h> 

#define MAX_LINES 100000
#define CHUNK_SIZE 1000
#define LINE_LENGTH 24
#define MAX_DIST 3466
#define FILENAME "cells"

struct coord {
    short n1; 
    short n2;
    short n3;
};

long read_file(char* lines[], char* filename);
void parse_coords(struct coord coords[], long num_coords, char* lines[]);
void compute_distances(long dist_counts[], long num_coords, struct coord coords[]);
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

    // printf("reading file ...\n");
    char* lines[MAX_LINES];
    long num_coords = read_file(lines, filename);

    // printf("parsing coords ...\n");
    struct coord coords[num_coords];
    parse_coords(coords, num_coords, lines);

    // printf("computing distances ...\n");
    long dist_counts[MAX_DIST];
    compute_distances(dist_counts, num_coords, coords);

    print_results(dist_counts);

    return 0;
}

long read_file(char* lines[], char* filename) {
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("could not open file %s\n", filename);
        exit(1);
    }

    char line[LINE_LENGTH];
    long num_lines = 0;
    while (fread(line, sizeof(char), LINE_LENGTH, fp) == LINE_LENGTH) { // TODO: no bufferoverflow -check
        lines[num_lines] = (char*) malloc(sizeof(char) * LINE_LENGTH);
        memcpy(lines[num_lines], line, sizeof(char) * LINE_LENGTH);
        num_lines++;
    }

    fclose(fp);

    return num_lines;
}

void parse_coords(struct coord coords[], long num_coords, char* lines[]) {
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
    for (size_t dist = 0; dist < MAX_DIST; dist++) {
        dist_counts[dist] = 0;
    }

    size_t num_chunks = num_coords / CHUNK_SIZE + 1;

    #pragma omp parallel for reduction(+:dist_counts[:MAX_DIST])
    for (size_t chunk_i = 0; chunk_i < num_chunks; chunk_i++) {
        size_t start_index_i = chunk_i * CHUNK_SIZE;

        // Calculate distances between all coords in chunk i
        for (size_t i1 = start_index_i; i1 < start_index_i + CHUNK_SIZE - 1 && i1 < num_coords; i1++) {
            for (size_t i2 = i1 + 1; i2 < start_index_i + CHUNK_SIZE && i2 < num_coords; i2++) {
                struct coord c1 = coords[i1];
                struct coord c2 = coords[i2];
                short dist = compute_distance(c1, c2);
                dist_counts[dist]++;
            }
        }

        for (size_t chunk_j = chunk_i + 1; chunk_j < num_chunks; chunk_j++) {
            // Calculate distances between all coords in chunks i and j
            for (size_t i = start_index_i; i < start_index_i + CHUNK_SIZE && i < num_coords; i++) {
                size_t start_index_j = chunk_j * CHUNK_SIZE;
                for (size_t j = start_index_j; j < start_index_j + CHUNK_SIZE && j < num_coords; j++) {
                    struct coord c1 = coords[i];
                    struct coord c2 = coords[j];
                    short dist = compute_distance(c1, c2);
                    dist_counts[dist]++;
                }
            }
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
        printf("%05.2f %ld\n", ((double) i) / 100.0, count);
    }
}
