#include "chess.h"

char Evaluate(void) {
  side = !side;
  moves possible_moves[1];
  Moves(possible_moves);

  side = !side;
  for (int move_count = 0; move_count < possible_moves->count; move_count++) {
    copy_board();
    side = !side;
    if (!makemove(possible_moves->moves[move_count], matecheck)) {
      take_back();
      continue;
    }
    take_back();
    return 'n';
  }

  if (is_square_attacked((side == white) ? bitScanForward(bitboards[K]) : bitScanForward(bitboards[k]), side)) {
    return 'c';
  }
  return 's';
}
void Moves(moves* possible_moves) {
  possible_moves->count = 0;
  int source_square, target_square;
  U64 bitboard, attacks;

  for (int piece = P; piece <= k; piece++) {
    bitboard = bitboards[piece];

    if (side == white) {
      if (piece == P) {
        while (bitboard) {
          source_square = bitScanForward(bitboard);

          target_square = source_square - 8;

          if (!(target_square < a8) && !getbit(occboards[both], target_square)) {
            if (source_square >= a7 && source_square <= h7) {
              addmove(possible_moves, encodemove(source_square, target_square, piece, Q, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, R, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, B, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, N, 0, 0, 0, 0));
            }
            else {
              addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));

              if ((source_square >= a2 && source_square <= h2) && !getbit(occboards[both], target_square - 8)) {
                addmove(possible_moves, encodemove(source_square, (target_square - 8), piece, 0, 0, 1, 0, 0));
              }
            }
          }

          attacks = pawn_attacks[side][source_square] & occboards[black];

          while (attacks) {
            target_square = bitScanForward(attacks);

            if (source_square >= a7 && source_square <= h7) {
              addmove(possible_moves, encodemove(source_square, target_square, piece, Q, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, R, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, B, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, N, 1, 0, 0, 0));
            }
            else {
              addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));
            }

            popbit(attacks, target_square);
          }

          if (enpassant != null_square) {
            U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

            if (enpassant_attacks) {
              int target_enpassant = bitScanForward(enpassant_attacks);
              addmove(possible_moves, encodemove(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
            }
          }

          popbit(bitboard, source_square);
        }
      }

      if (piece == K) {
        if (castle & wk) {
          if (!getbit(occboards[both], f1) && !getbit(occboards[both], g1)) {
            if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black)) {
              addmove(possible_moves, encodemove(e1, g1, piece, 0, 0, 0, 0, 1));
            }
          }
        }

        if (castle & wq) {
          if (!getbit(occboards[both], d1) && !getbit(occboards[both], c1) && !getbit(occboards[both], b1)) {
            if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black)) {
              addmove(possible_moves, encodemove(e1, c1, piece, 0, 0, 0, 0, 1));
            }
          }
        }
      }
    }
    else {
      if (piece == p) {
        while (bitboard) {
          source_square = bitScanForward(bitboard);

          target_square = source_square + 8;

          if (!(target_square > h1) && !getbit(occboards[both], target_square)) {
            if (source_square >= a2 && source_square <= h2) {
              addmove(possible_moves, encodemove(source_square, target_square, piece, q, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, r, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, b, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, n, 0, 0, 0, 0));
            }
            else {
              addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));

              if ((source_square >= a7 && source_square <= h7) && !getbit(occboards[both], target_square + 8)) {
                addmove(possible_moves, encodemove(source_square, (target_square + 8), piece, 0, 0, 1, 0, 0));
              }
            }
          }

          attacks = pawn_attacks[side][source_square] & occboards[white];

          while (attacks) {
            target_square = bitScanForward(attacks);

            if (source_square >= a2 && source_square <= h2) {
              addmove(possible_moves, encodemove(source_square, target_square, piece, q, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, r, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, b, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, n, 1, 0, 0, 0));
            }
            else {
              addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));
            }

            popbit(attacks, target_square);
          }

          if (enpassant != null_square) {
            U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

            if (enpassant_attacks) {
              int target_enpassant = bitScanForward(enpassant_attacks);
              addmove(possible_moves, encodemove(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
            }
          }

          popbit(bitboard, source_square);
        }
      }

      if (piece == k) {
        if (castle & bk) {
          if (!getbit(occboards[both], f8) && !getbit(occboards[both], g8)) {
            if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white)) {
              addmove(possible_moves, encodemove(e8, g8, piece, 0, 0, 0, 0, 1));
            }
          }
        }

        if (castle & bq) {
          if (!getbit(occboards[both], d8) && !getbit(occboards[both], c8) && !getbit(occboards[both], b8)) {
            if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white)) {
              addmove(possible_moves, encodemove(e8, c8, piece, 0, 0, 0, 0, 1));
            }
          }
        }
      }
    }

    if ((side == white) ? piece == N : piece == n) {
      while (bitboard) {
        source_square = bitScanForward(bitboard);

        attacks = knight_attacks[source_square] & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks) {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square)) {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));
          }
          else {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }

    if ((side == white) ? piece == B : piece == b) {
      while (bitboard) {
        source_square = bitScanForward(bitboard);

        attacks = get_bishop_attacks(source_square, occboards[both]) & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks) {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square)) {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));
          }
          else {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }

    if ((side == white) ? piece == R : piece == r) {
      while (bitboard) {
        source_square = bitScanForward(bitboard);

        attacks = get_rook_attacks(source_square, occboards[both]) & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks) {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square)) {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));
          }
          else {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }

    if ((side == white) ? piece == Q : piece == q) {
      while (bitboard) {
        source_square = bitScanForward(bitboard);

        attacks = get_queen_attacks(source_square, occboards[both]) & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks) {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square)) {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));
          }
          else {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }

    if ((side == white) ? piece == K : piece == k) {
      while (bitboard) {
        source_square = bitScanForward(bitboard);

        attacks = king_attacks[source_square] & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks) {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square)) {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));
          }
          else {
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }
  }
}
int Solve(int depth) {
  if (depth == 0) {
    nodes++;
    return 6;
  }

  moves possible_moves[1];
  memset(possible_moves, 0, sizeof(possible_moves));
  Moves(possible_moves);

  for (int move_count = 0; move_count < possible_moves->count; move_count++) {
    copy_board();

    if (getmovesource(possible_moves->moves[move_count]) == getmovetarget(possible_moves->moves[move_count])) {
      continue;
    }
    int result = makemove(possible_moves->moves[move_count], all_moves);
    // print_board(0);
    if (result == 0) {
      take_back();
      continue;
    }

    if (result == 2) {
      possible_moves->win = 1;
      // if the player will win, we don't need to look at further moves..
      // winning_move = possible_moves->moves[move_count];
      return side;
    }

    if (result == stale) {
      // if the move results in stalemate..
      possible_moves->draw = 1;
      return stale;
    }

    // if none of the above it must be a normal move and we continue on
    if (possible_moves->cont == 0) {
      possible_moves->cont = 1;
    }
    else {
      possible_moves->cont = 2;
    }

    side = !side;
    result = Solve(depth - 1);
    take_back();

    // if move comes back as a win
    if (result == side) {
      possible_moves->win = 1;
      // possible_moves->winning_move = possible_moves->moves[move_count];
    }
    else if (result == 5 || result == both) {
      // if not then we check if == draw..
      possible_moves->draw = 1;
      // cbranch.count -= 1;
    }
    else if (result == 6) {
      // any moves that aren't immediate draws / checkmates...
      possible_moves->cont = 1;
      // cbranch.count -= 1;
    }
    else {
      // and if no wins, draws, or continue playings then we must lose :(
      possible_moves->lose = 1;
      if (possible_moves->cont > 0) {
        possible_moves->cont -= 1;
      }
      // possible_moves->winning_move = possible_moves->moves[move_count];
    }
  }

  // after the above has been done,  we check if we have any wins
  // if we do, we obviously want to play them, so we return this node with a win for the current side..
  if (possible_moves->win > 0) {
    // print_move(possible_moves->winning_move);
    return side;
  }
  // if their are continues
  if (possible_moves->cont > 0) {
    return contin;
  }
  // if no wins, and no continues we dont want opponent to win, so we return a draw if their are any..
  if (possible_moves->draw > 0) {
    return both;
  }
  // and if no wins draws or continues, but there are losses, then opponent must win :(
  if (possible_moves->lose > 0) {
    // print_move(winning_move);
    return !side;
  }

  // we would get to this point if there were no legal moves evaluated..
  // so either the position is already in checkmate
  // or the position is a stalemate.., so we check -> is it a checkmate by seeing if our our king is attacked
  if (is_square_attacked((side == white) ? bitScanForward(bitboards[K]) : bitScanForward(bitboards[k]), side)) {
    return !side;
  }
  else {
    // and if not attacked then its a stalemate
    return both;
  }
}

#define FEN "2bqkbn1/2pppp2/np2N3/r3P1p1/p2N2B1/5Q2/PPPPKPP1/RNB2r2 w KQkq - 0 1"
int main(int argc , char **argv) {
  init_all();
  unsigned long long moves_looked_at = __LONG_LONG_MAX__;
  int start;
  int end;
  for (unsigned long long i = 1, n = moves_looked_at * 2; i < n; i++) {
    parse_fen(FEN_START);
    printf("%llu: ", i);

    start = get_time_ms();
    int result = Solve(i);
    end = get_time_ms();

    printf("(%i ms) ", (end - start));
    if (result == contin) {
      printf("Nothing yet..\n");
    } else if (result == black) {
      printf("Black wins :)\n");
      getchar();
      return 0;
    } else if (result == white) {
      printf("White wins :)\n");
      getchar();
      return 0;
    } else {
      printf("Draw :|\n");
      getchar();
      return 0;
    }
  }
  return 0;
}