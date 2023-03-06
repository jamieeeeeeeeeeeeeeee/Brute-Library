#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Game state representation
#define DRAW 0
#define O -1
#define X 1 // X is maximiser
#define NO_MOVE 2
#define POS_INF 1000000
#define NEG_INF -1000000

typedef struct position {
  unsigned x : 9;
  unsigned o : 9;
  unsigned turn : 1;
} position;

typedef struct moveset {
  position moves[9];
  int count;
} moveset;

#define setbit(bitboard, square) ((bitboard) |= (1 << (square)))
#define getbit(bitboard, square) ((bitboard) & (1 << (square)))
#define popbit(bitboard, square) ((bitboard) &= ~(1 << (square)))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

// Helper functions
int count_bits(unsigned x) {
  #ifdef __builtin_popcount
  return __builtin_popcount(x);
  #else
  int count = 0;
  while (x) {
    count++;
    x &= (x - 1);
  }
  return count;
  #endif
}
void print_position(position P) {
  printf("\n");
  for (int i = 0; i < 9; i++) {
    if (getbit(P.x, i)) {
      printf("x ");
    } else if (getbit(P.o, i)) {
      printf("o ");
    } else { 
      printf("- ");
    }
    if ((i + 1) % 3 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}

// Prototypes
void Moves(position P, position *M);
int Evaluate(position P);
int Solve(position P, int alpha, int beta);