#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <errno.h>

char unique_digits(unsigned long long n);
char num_in_descending_order(unsigned long long n);

// Sieve of Eratosthenes
int main(int argc, char** argv) {
  clock_t start = clock();
  unsigned long long MAX = 9999999ULL;
  if (argc == 2) {
    MAX = strtoull(argv[1], NULL, 10);
    if (errno == ERANGE) {
      printf("Error: strtoull failed - your number is too big!\n");
      return 1;
    }
  }

  uint8_t* array = malloc((MAX / 8 + 1) * sizeof(uint8_t));
  if (array == NULL) {
    printf("Error: malloc failed - not enough memory.\n");
    return 1;
  }

  uint8_t pow2[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

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
    printf("> %llu \r", divisor);

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
  int special_nums = 0;
  int really_special_nums = 0;
  int special_primes = 0;
  int really_special_primes = 0;
  int failures = 0;
  for (i = 2; i < MAX; i++) {
    if (1) {//(unique_digits(i) && num_in_descending_order(i))) {
      special_nums += 1;
      if ((array[i / 8] & (pow2[i % 8]))) {
        special_primes += 1;
      }
      int digits[i > 0 ? (int)log10((double)i) + 1 : 1];
      int j = 0;
      unsigned long long n = i;
      while (n > 0) {
        digits[j++] = n % 10;
        n /= 10;
      }
      int set[] = { 2 };
      unsigned long long total;
      for (int s = 0, p = sizeof(set) / sizeof(int); s < p ; s++) {
        total = 1;
        for (int k = 0; k < j; k++) {
          total += pow(set[s], digits[k]);
        }
        //seg fault comes from checking that they are prime or not, not pow!
        if (total > MAX) { // no need for < 0 because unsigned
          failures += 1;
          printf("\033[31m%i\033[0m ", failures);
          printf("\033[31m%llu >> Total too big for index! >> %llu (%i)\n\033[0m", i, total, set[s]);
          break;
        }
        if (array[total / 8] & (pow2[total % 8])) {
          //printf("\033[32m%llu (%i + 1)\n\033[0m", i, set[s]);
          if ((array[i / 8] & (pow2[i % 8]))) {
            really_special_primes += 1;
          } 
          really_special_nums += 1;
          break;
        } else if (array[(total - 2) / 8] & (pow2[(total - 2) % 8])){
          //printf("\033[32m%llu (%i - 1)\n\033[0m", i, set[s]);
          if ((array[i / 8] & (pow2[i % 8]))) {
            really_special_primes += 1;
          } 
          really_special_nums += 1;
          break;
        } else if (s == p - 1){
          printf("\033[31m%llu \n\033[0m", i);
        }
      }
    }
  }

  free(array);
  printf("\n\033[34mFound %i special primes and %i really special primes.\033[0m\n", special_primes, really_special_primes);
  printf("\n\033[34mFound %i special numbers and %i really special numbers.\033[0m\n", special_nums, really_special_nums);
  return 0;
}

char unique_digits(unsigned long long n) {
  char digits[10] = { 0 };
  while (n > 0) {
    if (digits[n % 10] == 1) {
      return 0;
    }
    digits[n % 10] = 1;
    n /= 10;
  }
  return 1;
}

char num_in_descending_order(unsigned long long n) {
  unsigned long long last_digit = 0;
  while (n > 0) {
    if (n % 10 < last_digit) {
      return 0;
    }
    last_digit = n % 10;
    n /= 10;
  }
  return 1;
}