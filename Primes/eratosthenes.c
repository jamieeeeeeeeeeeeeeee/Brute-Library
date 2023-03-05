#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>

// Sieve of Eratosthenes
int main(int argc, char **argv) {
    clock_t start = clock();
    unsigned long long MAX = 999999999ULL; // ~10 GB memory
    if (argc == 2) {
        MAX = strtoull(argv[1], NULL, 10);
        if (errno == ERANGE) {
            printf("Error: strtoull failed - your number is too big!\n");
            return 1;
        }
    }

    uint8_t *array = malloc((MAX / 8 + 1) * sizeof(uint8_t));
    if (array == NULL) {
        printf("Error: malloc failed - not enough memory.\n");
        return 1;
    }

    uint8_t pow2[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    
    unsigned long long i;
    unsigned long long divisor = 1;
    unsigned long long sqrt_max = sqrt(MAX);

    memset(array, 255, (MAX / 8 + 1) * sizeof(uint8_t));

    array[0] &= 0xFC; // 0 and 1 are not prime

    while (divisor < sqrt_max) {
        i = divisor + 1;
        while (i < MAX) {
            if ((array[i / 8] & (pow2[i % 8])) == 0) {
                i++;
                continue;
            }
            break;
        }
        divisor = i;
        //printf("> %llu \r", divisor);

        i += divisor;
        while (i < MAX) {
            array[i / 8] &= ~(pow2[i % 8]);
            i += divisor;
        }
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("\033[32m\nSearch completed! %0.f ms\033[0m\n\n", elapsed);
    for (i = 0; i < MAX; i++) {
        if ((array[i / 8] & (pow2[i % 8])) != 0) {
            divisor = i;
            //printf("\033[32m%llu\033[0m ", divisor);
        }
    }

    printf("\033[34mBiggest Prime (< %llu) is %llu\033[0m\n", MAX, divisor);
    free(array);
    return 0;
}
