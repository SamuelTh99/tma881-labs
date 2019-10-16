#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h> 

int main() {
    int counts[10];
    for (int i = 0; i < 10; i++) {
        counts[i] = 0;
    }
    #pragma omp parallel for reduction(+:counts[:10])
    for (int i = 0; i < 10000; i++) {
        for (int j = 0; j < 10000; j++) {
            counts[(i + j) % 10]++;
        }
    }
    for (int i = 0; i < 10; i++) {
        printf("%d\n", counts[i]);
    }
}