#include <stdio.h>
#include <stdlib.h>

char is_sorted(int* array, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (array[i] > array[i + 1]) {
            return 0;
        }
    }
    return 1;
}

unsigned long long bogosort(int* array, int size) {
    unsigned long long t = 0;
    int temp = 0;
    while (!is_sorted(array, size)) {
        for (int i = 0; i < size; i++) {
            int j = rand() % size;
            temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
        t += 1;
    }
    return t;
}

int main(int argc, char **argv) {
    int N = 10;
    if (argc = 2) {
        N = atoi(argv[1]); //unsafe
    }
    int *array = malloc(sizeof(int) * N);
    for (int i = 0; i < N; i++) {
        array[i] = rand() % 100;
    }
    unsigned long long t = bogosort(array, N);
    printf("Attempts: %lld\n", t);
    for (int i = 0; i < N; i++) {
        printf("%d ", array[i]);
    }
    free(array);
}