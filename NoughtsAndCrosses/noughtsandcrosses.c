#include "noughtsandcrosses.h"

void Moves(position P, position *M) {
  int count = 0;
  if (P.turn == X) {
    for (int i = 0; i < 9; i++) {
      if (getbit(P.x, i) || getbit(P.o, i)) {
        continue;
      } else {
        memcpy(&M[count], &P, sizeof(position));
        setbit(M[count++].x, i);
      }
    }
  } else {
    for (int i = 0; i < 9; i++) {
      if (getbit(P.x, i) || getbit(P.o, i)) {
        continue;
      } else {
        memcpy(&M[count], &P, sizeof(position));
        setbit(M[count++].o, i);
      }
    }
  }
  return;
}
int Evaluate(position P) {
  for (int row = 0; row < 3; row++) {
    if (getbit(P.x, row * 3) && getbit(P.x, row * 3 + 1) && getbit(P.x, row * 3 + 2)) {
      return X;
    }
    if (getbit(P.o, row * 3) && getbit(P.o, row * 3 + 1) && getbit(P.o, row * 3 + 2)) {
      return O;
    }
  }
  for (int col = 0; col < 3; col++) {
    if (getbit(P.x, col) && getbit(P.x, col + 3) && getbit(P.x, col + 6)) {
      return X;
    }
    if (getbit(P.o, col) && getbit(P.o, col + 3) && getbit(P.o, col + 6)) {
      return O;
    }
  }
  if ((getbit(P.x, 0) && getbit(P.x, 4) && getbit(P.x, 8)) || (getbit(P.x, 2) && getbit(P.x, 4) && getbit(P.x, 6))) {
    return X;
  }
  if ((getbit(P.o, 0) && getbit(P.o, 4) && getbit(P.o, 8)) || (getbit(P.o, 2) && getbit(P.o, 4) && getbit(P.o, 6))) {
    return O;
  }

  return DRAW;
}
int Solve(position P, int alpha, int beta) {
  int result = Evaluate(P);
  if (result != DRAW) {
    return result;
  }

  if (count_bits(P.x) + count_bits(P.o) >= 9) {
    return DRAW;
  }

  position *M = malloc(9 * sizeof(position));
  if (M == NULL){
    printf("Malloc failure: M\n");
    return 1;
  }
  memset(M, 0, 9 * sizeof(position));

  Moves(P, M);

  char *evals = malloc(9 * sizeof(int));
  if (evals == NULL) {
    printf("Malloc failure: evals\n");
    free(M);
    return 1;
  }
  memset(evals, NO_MOVE, 9 * sizeof(int));

  for (int i = 0; i < 9; i++) {
    if (M[i].x == 0 && M[i].o == 0) {
      continue;
    }
    M[i].turn = !M[i].turn;
    evals[i] = Solve(M[i], alpha, beta);
    if (P.turn == X) {
      if (evals[i] > alpha) {
        alpha = evals[i];
      }
    } else {
      if (evals[i] < beta) {
        beta = evals[i];
      }
    }
    if (alpha >= beta) {
      break;
    }
  }

  // == instead of != because we changed the move flag earlier
  if (P.turn == X) {
    free(M);
    free(evals);
    return alpha;
  } else {
    free(M);
    free(evals);
    return beta;
  }
}

int main(int argc, char **argv) {
  position START = {0, 0, X};
  if (argc == 2) {
    for (int i = 0; i < 9; i++) {
      if (argv[1][i] == '\0') {
        printf("Invalid input length\n");
        return 1;
      } 
      switch(argv[1][i]) {
        case 'x':
          setbit(START.x, i);
          break;
        case 'o':
          setbit(START.o, i);
          break;
        case 'X':
          setbit(START.x, i);
          break;
        case 'O':
          setbit(START.o, i);
          break;
        default:
          break;
      }
    }
  }  
  int result = Solve(START, NEG_INF, POS_INF);
  printf("%d\n", result);
  if (result == DRAW) {
    printf("Draw\n");
  } else {
    printf("%c wins\n", result == X ? 'X' : 'O');
  }

  return 0;
}
