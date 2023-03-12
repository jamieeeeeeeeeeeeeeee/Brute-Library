// CPP program to check for primality using
// Lucas-Lehmer series.
#include <cmath>
#include <iostream>
using namespace std;

// Function to check whether (2^p - 1)
// is prime or not.
bool isPrime(int p) {
  // generate the number
  long long checkNumber = pow(2, p) - 1;

  // First number of the series
  long long nextval = 4 % checkNumber;

  // Generate the rest (p-2) terms
  // of the series.
  for (int i = 1; i < p - 1; i++) {
      nextval = (nextval * nextval - 2) % checkNumber;
  }

  // now if the (p-1)th term is
  // 0 return true else false.
  return (nextval == 0);
}

// Driver Program
int main(int argc, char** argv) {
  // Check whether 2^p-1 is prime or not.
  if (argc != 2) {
    cout << "Usage: " << argv[0] << " p" << endl;
    return 1;
  }
  int p = atoi(argv[1]);

  long long checkNumber = pow(2, p) - 1;

  if (isPrime(p)) {
      cout << checkNumber << " is Prime.";
  } else {
      cout << checkNumber << " is not Prime.";
  }

  return 0;
}