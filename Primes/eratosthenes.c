#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>

// Sieve of Eratosthenes
int main(int argc, char **argv) {
    unsigned long long MAX = 99999999999ULL; // ~10 GB memory
    if (argc == 2) {
        MAX = strtoull(argv[1], NULL, 10);
        if (errno == ERANGE) {
            printf("Error: strtoull failed - your number is too big!\n");
            return 1;
        }
    }

    uint8_t *array = malloc((MAX / 8 + 1) * sizeof(uint8_t));
    if (array == NULL) {
        printf("Error: malloc failed - you don't have enough memory!\n");
        return 1;
    }
    unsigned long long i = 0;
    unsigned long long divisor = 1;
    unsigned long long sqrt_max = sqrt(MAX);

    memset(array, 255, (MAX / 8 + 1) * sizeof(uint8_t));

    array[0] &= 0xfe;
    array[1] &= 0xfd;

    while (divisor < sqrt_max) {
        i = divisor + 1;
        while (i < MAX) {
            if ((array[i / 8] & (1 << (i % 8))) == 0) {
                i++;
                continue;
            }
            break;
        }
        divisor = i;
        printf("> %llu \r", divisor);

        i += divisor;
        while (i < MAX) {
            array[i / 8] &= ~(1 << (i % 8));
            i += divisor;
        }
    }

    printf("\033[32m\nSearch completed!\033[0m\n\n");
    for (i = 0; i < MAX; i++) {
        if ((array[i / 8] & (1 << (i % 8))) != 0) {
            divisor = i;
            // printf("\033[32m%llu\033[0m ", divisor);
        }
    }

    printf("\033[34mBiggest Prime (< %llu) is %llu\033[0m\n", MAX, divisor);
    free(array);
    return 0;
}
