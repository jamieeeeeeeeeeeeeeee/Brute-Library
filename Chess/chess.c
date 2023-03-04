#include "chess.h"

U64 pawn_attacks[2][64];
U64 knight_attacks[64];
U64 king_attacks[64];
U64 bishop_masks[64];
U64 rook_masks[64];
U64 bishop_attacks[64][512];
U64 rook_attacks[64][4096];
U64 bitboards[12];
U64 occboards[3];
int side;
int enpassant = null_square;
int castle;
unsigned long long nodes;

char checkmateresult;
int winning_move;
char PYFEN[128];

static inline int count_bits(U64 bitboard)
{
  int count = 0;

  while (bitboard)
  {
    count++;
    bitboard &= bitboard - 1;
  }

  return count;
}
void print_bitboard(U64 bitboard)
{
  printf("\n");

  for (int rank = 0; rank < 8; rank++)
  {
    for (int file = 0; file < 8; file++)
    {
      int square = rank * 8 + file;

      if (!file)
        printf("  %d ", 8 - rank);
      printf(" %d", getbit(bitboard, square) ? 1 : 0);
    }
    printf("\n");
  }

  printf("\n     a b c d e f g h\n\n");

  printf("     Bitboard: %llud\n\n", bitboard);
}
void print_board(char info)
{
  printf("\n");

  for (int rank = 0; rank < 8; rank++)
  {
    for (int file = 0; file < 8; file++)
    {
      int square = rank * 8 + file;
      if (!file)
        printf("  %d ", 8 - rank);
      int piece = -1;

      for (int bb_piece = P; bb_piece <= k; bb_piece++)
      {
        if (getbit(bitboards[bb_piece], square))
          piece = bb_piece;
      }

      // #ifdef WIN64
      printf(" %c", (piece == -1) ? '.' : ascii_pieces[piece]);
      // #else
      //     printf(" %s", (piece == -1) ? "." : unicode_pieces[piece]);
      // #endif
    }

    printf("\n");
  }

  printf("\n     a b c d e f g h\n\n");

  if (info)
  {
    printf("     Side:     %s\n", !side ? "white" : "black");
    printf("     Enpassant:   %s\n", (enpassant != null_square) ? square_to_coordinates[enpassant] : "no");
    printf("     Castling:  %c%c%c%c\n\n", (castle & wk) ? 'K' : '-',
           (castle & wq) ? 'Q' : '-',
           (castle & bk) ? 'k' : '-',
           (castle & bq) ? 'q' : '-');
  }
}
void parse_fen(char *fen)
{
  memset(bitboards, 0ULL, sizeof(bitboards));
  memset(occboards, 0ULL, sizeof(occboards));

  side = 0;
  enpassant = null_square;
  castle = 0;

  for (int rank = 0; rank < 8; rank++)
  {
    for (int file = 0; file < 8; file++)
    {
      int square = rank * 8 + file;

      if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
      {
        int piece = char_pieces[*fen];
        setbit(bitboards[piece], square);

        fen++;
      }

      if (*fen >= '0' && *fen <= '9')
      {
        int offset = *fen - '0';
        int piece = -1;

        for (int bb_piece = P; bb_piece <= k; bb_piece++)
        {
          if (getbit(bitboards[bb_piece], square))
            piece = bb_piece;
        }

        if (piece == -1)
          file--;

        file += offset;

        fen++;
      }

      if (*fen == '/')
        fen++;
    }
  }

  fen++;

  (*fen == 'w') ? (side = white) : (side = black);

  fen += 2;

  while (*fen != ' ')
  {
    switch (*fen)
    {
    case 'K':
      castle |= wk;
      break;
    case 'Q':
      castle |= wq;
      break;
    case 'k':
      castle |= bk;
      break;
    case 'q':
      castle |= bq;
      break;
    case '-':
      break;
    }

    fen++;
  }

  fen++;

  if (*fen != '-')
  {
    int file = fen[0] - 'a';
    int rank = 8 - (fen[1] - '0');

    enpassant = rank * 8 + file;
  }

  else
    enpassant = null_square;

  for (int piece = P; piece <= K; piece++)
    occboards[white] |= bitboards[piece];

  for (int piece = p; piece <= k; piece++)
    occboards[black] |= bitboards[piece];

  occboards[both] |= occboards[white];
  occboards[both] |= occboards[black];
  print_board(1);
}
void gen_fen(void)
{
  int index = 0;

  for (int rank = 0; rank < 8; rank++)
  {
    int empty = 0;

    for (int file = 0; file < 8; file++)
    {
      int square = rank * 8 + file;
      int piece = -1;

      for (int bb_piece = P; bb_piece <= k; bb_piece++)
      {
        if (getbit(bitboards[bb_piece], square))
          piece = bb_piece;
      }

      if (piece == -1)
        empty++;

      else
      {
        if (empty)
        {
          PYFEN[index++] = '0' + empty;
          empty = 0;
        }

        PYFEN[index++] = ascii_pieces[piece];
      }
    }

    if (empty)
      PYFEN[index++] = '0' + empty;

    if (rank != 7)
      PYFEN[index++] = '/';
  }

  PYFEN[index++] = ' ';
  PYFEN[index++] = (side == white) ? 'w' : 'b';
  PYFEN[index++] = ' ';

  if (castle & wk)
    PYFEN[index++] = 'K';

  if (castle & wq)
    PYFEN[index++] = 'Q';

  if (castle & bk)
    PYFEN[index++] = 'k';

  if (castle & bq)
    PYFEN[index++] = 'q';

  if (!(castle & wk) && !(castle & wq) && !(castle & bk) && !(castle & bq))
    PYFEN[index++] = '-';

  PYFEN[index++] = ' ';

  if (enpassant != null_square)
  {
    PYFEN[index++] = square_to_coordinates[enpassant][0];
    PYFEN[index++] = square_to_coordinates[enpassant][1];
  }

  else
    PYFEN[index++] = '-';

  PYFEN[index] = '\0';
  printf("%s", PYFEN);
}
U64 mask_pawn_attacks(int side, int square)
{
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  setbit(bitboard, square);

  if (!side)
  {
    if ((bitboard >> 7) & NOTAFILE)
      attacks |= (bitboard >> 7);
    if ((bitboard >> 9) & NOTHFILE)
      attacks |= (bitboard >> 9);
  }

  else
  {
    if ((bitboard << 7) & NOTHFILE)
      attacks |= (bitboard << 7);
    if ((bitboard << 9) & NOTAFILE)
      attacks |= (bitboard << 9);
  }

  return attacks;
}
U64 mask_knight_attacks(int square)
{
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  setbit(bitboard, square);

  if ((bitboard >> 17) & NOTHFILE)
    attacks |= (bitboard >> 17);
  if ((bitboard >> 15) & NOTAFILE)
    attacks |= (bitboard >> 15);
  if ((bitboard >> 10) & NOTHGFILE)
    attacks |= (bitboard >> 10);
  if ((bitboard >> 6) & NOTABFILE)
    attacks |= (bitboard >> 6);
  if ((bitboard << 17) & NOTAFILE)
    attacks |= (bitboard << 17);
  if ((bitboard << 15) & NOTHFILE)
    attacks |= (bitboard << 15);
  if ((bitboard << 10) & NOTABFILE)
    attacks |= (bitboard << 10);
  if ((bitboard << 6) & NOTHGFILE)
    attacks |= (bitboard << 6);

  return attacks;
}
U64 mask_king_attacks(int square)
{
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  setbit(bitboard, square);

  if (bitboard >> 8)
    attacks |= (bitboard >> 8);
  if ((bitboard >> 9) & NOTHFILE)
    attacks |= (bitboard >> 9);
  if ((bitboard >> 7) & NOTAFILE)
    attacks |= (bitboard >> 7);
  if ((bitboard >> 1) & NOTHFILE)
    attacks |= (bitboard >> 1);
  if (bitboard << 8)
    attacks |= (bitboard << 8);
  if ((bitboard << 9) & NOTAFILE)
    attacks |= (bitboard << 9);
  if ((bitboard << 7) & NOTHFILE)
    attacks |= (bitboard << 7);
  if ((bitboard << 1) & NOTAFILE)
    attacks |= (bitboard << 1);

  return attacks;
}
U64 mask_bishop_attacks(int square)
{
  U64 attacks = 0ULL;

  int r, f;
  int tr = square / 8;
  int tf = square % 8;

  for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++)
    attacks |= (1ULL << (r * 8 + f));
  for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++)
    attacks |= (1ULL << (r * 8 + f));
  for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--)
    attacks |= (1ULL << (r * 8 + f));
  for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--)
    attacks |= (1ULL << (r * 8 + f));

  return attacks;
}
U64 mask_rook_attacks(int square)
{
  U64 attacks = 0ULL;

  int r, f;

  int tr = square / 8;
  int tf = square % 8;

  for (r = tr + 1; r <= 6; r++)
    attacks |= (1ULL << (r * 8 + tf));
  for (r = tr - 1; r >= 1; r--)
    attacks |= (1ULL << (r * 8 + tf));
  for (f = tf + 1; f <= 6; f++)
    attacks |= (1ULL << (tr * 8 + f));
  for (f = tf - 1; f >= 1; f--)
    attacks |= (1ULL << (tr * 8 + f));

  return attacks;
}
U64 bishop_attacks_on_the_fly(int square, U64 block)
{
  U64 attacks = 0ULL;

  int r, f;
  int tr = square / 8;
  int tf = square % 8;

  for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
  {
    attacks |= (1ULL << (r * 8 + f));
    if ((1ULL << (r * 8 + f)) & block)
      break;
  }

  for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
  {
    attacks |= (1ULL << (r * 8 + f));
    if ((1ULL << (r * 8 + f)) & block)
      break;
  }

  for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
  {
    attacks |= (1ULL << (r * 8 + f));
    if ((1ULL << (r * 8 + f)) & block)
      break;
  }

  for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
  {
    attacks |= (1ULL << (r * 8 + f));
    if ((1ULL << (r * 8 + f)) & block)
      break;
  }

  return attacks;
}
U64 rook_attacks_on_the_fly(int square, U64 block)
{
  U64 attacks = 0ULL;

  int r, f;
  int tr = square / 8;
  int tf = square % 8;

  for (r = tr + 1; r <= 7; r++)
  {
    attacks |= (1ULL << (r * 8 + tf));
    if ((1ULL << (r * 8 + tf)) & block)
      break;
  }

  for (r = tr - 1; r >= 0; r--)
  {
    attacks |= (1ULL << (r * 8 + tf));
    if ((1ULL << (r * 8 + tf)) & block)
      break;
  }

  for (f = tf + 1; f <= 7; f++)
  {
    attacks |= (1ULL << (tr * 8 + f));
    if ((1ULL << (tr * 8 + f)) & block)
      break;
  }

  for (f = tf - 1; f >= 0; f--)
  {
    attacks |= (1ULL << (tr * 8 + f));
    if ((1ULL << (tr * 8 + f)) & block)
      break;
  }

  return attacks;
}
void init_leapers_attacks(void)
{
  for (int square = 0; square < 64; square++)
  {
    pawn_attacks[white][square] = mask_pawn_attacks(white, square);
    pawn_attacks[black][square] = mask_pawn_attacks(black, square);
    knight_attacks[square] = mask_knight_attacks(square);
    king_attacks[square] = mask_king_attacks(square);
  }
}
U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask)
{
  U64 occupancy = 0ULL;

  for (int count = 0; count < bits_in_mask; count++)
  {
    int square = bitScanForward(attack_mask);
    popbit(attack_mask, square);

    if (index & (1 << count))
      occupancy |= (1ULL << square);
  }

  return occupancy;
}
void init_sliders_attacks(int bishop)
{
  for (int square = 0; square < 64; square++)
  {
    bishop_masks[square] = mask_bishop_attacks(square);
    rook_masks[square] = mask_rook_attacks(square);

    U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];
    int relevant_bits_count = count_bits(attack_mask);
    int occupancy_indicies = (1 << relevant_bits_count);

    for (int index = 0; index < occupancy_indicies; index++)
    {
      // bishop
      if (bishop)
      {
        U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
        int magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]);
        bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
      }

      else
      {
        U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
        int magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);
        rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
      }
    }
  }
}
static inline U64 get_bishop_attacks(int square, U64 occupancy)
{
  occupancy &= bishop_masks[square];
  occupancy *= bishop_magic_numbers[square];
  occupancy >>= 64 - bishop_relevant_bits[square];

  return bishop_attacks[square][occupancy];
}
static inline U64 get_rook_attacks(int square, U64 occupancy)
{
  occupancy &= rook_masks[square];
  occupancy *= rook_magic_numbers[square];
  occupancy >>= 64 - rook_relevant_bits[square];

  return rook_attacks[square][occupancy];
}
static inline U64 get_queen_attacks(int square, U64 occupancy)
{
  U64 queen_attacks = 0ULL;
  U64 bishop_occupancy = occupancy;
  U64 rook_occupancy = occupancy;

  bishop_occupancy &= bishop_masks[square];
  bishop_occupancy *= bishop_magic_numbers[square];
  bishop_occupancy >>= 64 - bishop_relevant_bits[square];

  queen_attacks = bishop_attacks[square][bishop_occupancy];

  rook_occupancy &= rook_masks[square];
  rook_occupancy *= rook_magic_numbers[square];
  rook_occupancy >>= 64 - rook_relevant_bits[square];

  queen_attacks |= rook_attacks[square][rook_occupancy];
  return queen_attacks;
}
static inline int is_square_attacked(int square, int side)
{
  if ((side == white) && (pawn_attacks[black][square] & bitboards[P]))
  {
    return 1;
  }
  if ((side == black) && (pawn_attacks[white][square] & bitboards[p]))
  {
    return 1;
  }
  if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n]))
  {
    ;
    return 1;
  }
  if (get_bishop_attacks(square, occboards[both]) & ((side == white) ? bitboards[B] : bitboards[b]))
  {
    return 1;
  }
  if (get_rook_attacks(square, occboards[both]) & ((side == white) ? bitboards[R] : bitboards[r]))
  {
    return 1;
  }
  if (get_queen_attacks(square, occboards[both]) & ((side == white) ? bitboards[Q] : bitboards[q]))
  {
    return 1;
  }
  return (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k]));
}
void print_move(int move)
{
  if (getmovepromoted(move))
    printf("%s%s%c\n", unicode_pieces[getmovepiece(move)],
           square_to_coordinates[getmovetarget(move)],
           promoted_pieces[getmovepromoted(move)]);
  else
    printf("%s%s\n", unicode_pieces[getmovepiece(move)],
           square_to_coordinates[getmovetarget(move)]);
}
void print_possible_moves(moves *possible_moves)
{
  if (!possible_moves->count)
  {
    printf("\n     No move in the move list!\n");
    return;
  }

  printf("\n     move    piece     capture   double    enpass    castling\n\n");

  for (int move_count = 0; move_count < possible_moves->count; move_count++)
  {
    int move = possible_moves->moves[move_count];
    printf("      %s%s%c   %c         %d         %d         %d         %d\n", square_to_coordinates[getmovesource(move)],
           square_to_coordinates[getmovetarget(move)],
           getmovepromoted(move) ? promoted_pieces[getmovepromoted(move)] : ' ',
           ascii_pieces[getmovepiece(move)],
           getmovecapture(move) ? 1 : 0,
           getmovedouble(move) ? 1 : 0,
           getmoveenpassant(move) ? 1 : 0,
           getmovecastling(move) ? 1 : 0);

    printf("     %s%s%c   %s         %d         %d         %d         %d\n", square_to_coordinates[getmovesource(move)],
           square_to_coordinates[getmovetarget(move)],
           getmovepromoted(move) ? promoted_pieces[getmovepromoted(move)] : ' ',
           unicode_pieces[getmovepiece(move)],
           getmovecapture(move) ? 1 : 0,
           getmovedouble(move) ? 1 : 0,
           getmoveenpassant(move) ? 1 : 0,
           getmovecastling(move) ? 1 : 0);
  }

  printf("\n\n     Total number of moves: %llu\n\n", possible_moves->count);
}
int get_time_ms(void)
{
  // #ifdef WIN64
  return GetTickCount();
  // #else
  //     struct timeval time_value;
  //     gettimeofday(&time_value, NULL);
  //     return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
  // #endif
}
void init_all(void)
{
  init_leapers_attacks();

  init_sliders_attacks(bishop);
  init_sliders_attacks(rook);

  parse_fen(FEN_START);
  // init_magic_numbers();
}
char checkorstale(void)
{
  side = !side;
  moves possible_moves[1];
  Moves(possible_moves);

  side = !side;
  for (int move_count = 0; move_count < possible_moves->count; move_count++)
  {
    copy_board();
    side = !side;
    if (!makemove(possible_moves->moves[move_count], matecheck))
    {
      take_back();
      continue;
    }
    take_back();
    return 'n';
  }

  if (is_square_attacked((side == white) ? bitScanForward(bitboards[K]) : bitScanForward(bitboards[k]), side))
  {
    return 'c';
  }
  return 's';
}

static inline int makemove(int move, int move_flag)
{
  int source_square = getmovesource(move);
  int target_square = getmovetarget(move);
  int piece = getmovepiece(move);
  int promoted_piece = getmovepromoted(move);
  int capture = getmovecapture(move);
  int double_push = getmovedouble(move);
  int enpass = getmoveenpassant(move);
  int castling = getmovecastling(move);

  popbit(bitboards[piece], source_square);
  setbit(bitboards[piece], target_square);

  if (capture)
  {
    // print_board(1);
    // getchar();
    int start_piece, end_piece;

    if (side == white)
    {
      start_piece = p;
      end_piece = k;
    }

    else
    {
      start_piece = P;
      end_piece = K;
    }

    for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
    {
      if (getbit(bitboards[bb_piece], target_square))
      {
        popbit(bitboards[bb_piece], target_square);
        break;
      }
    }
  }

  if (promoted_piece)
  {
    popbit(bitboards[(side == white) ? P : p], target_square);

    setbit(bitboards[promoted_piece], target_square);
  }

  if (enpass)
  {
    (side == white) ? popbit(bitboards[p], target_square + 8) : popbit(bitboards[P], target_square - 8);
  }

  enpassant = null_square;

  if (double_push)
  {
    (side == white) ? (enpassant = target_square + 8) : (enpassant = target_square - 8);
  }

  if (castling)
  {
    switch (target_square)
    {
    case (g1):
      popbit(bitboards[R], h1);
      setbit(bitboards[R], f1);
      break;

    case (c1):
      popbit(bitboards[R], a1);
      setbit(bitboards[R], d1);
      break;

    case (g8):
      popbit(bitboards[r], h8);
      setbit(bitboards[r], f8);
      break;

    case (c8):
      popbit(bitboards[r], a8);
      setbit(bitboards[r], d8);
      break;
    }
  }

  castle &= castling_rights[source_square];
  castle &= castling_rights[target_square];

  memset(occboards, 0ULL, 24);

  for (int bb_piece = P; bb_piece <= K; bb_piece++)
    occboards[white] |= bitboards[bb_piece];

  for (int bb_piece = p; bb_piece <= k; bb_piece++)
    occboards[black] |= bitboards[bb_piece];

  occboards[both] |= occboards[white];
  occboards[both] |= occboards[black];

  /*if (move_flag == all_moves)
  {
      print_board(0);
      getchar();
  }*/
  // print_board(0);

  if (is_square_attacked((side == white) ? bitScanForward(bitboards[K]) : bitScanForward(bitboards[k]), !side))
  {
    return 0;
  }

  if (move_flag == all_moves)
  {

    // print_move(move);
    /*getchar();
    if (getmovetarget(move) == f6)
        printf("stop\n");*/

    checkmateresult = checkorstale();
    if (checkmateresult == 'c')
    {
      // print_board(0);
      // printf("Checkmate with ");
      // print_move(move);
      // getchar();
      return 2;
    }
    else if (checkmateresult == 's')
    {
      // double check stalemate checking
      return 5;
      // print_board(0);
      printf("Stalemate with");
      print_move(move);
      return 5;
    }
  }
  return 1;
}

// Representation of the game state:

static inline void Moves(moves *possible_moves)
{
  possible_moves->count = 0;
  int source_square, target_square;
  U64 bitboard, attacks;

  for (int piece = P; piece <= k; piece++)
  {
    bitboard = bitboards[piece];

    if (side == white)
    {
      if (piece == P)
      {
        while (bitboard)
        {
          source_square = bitScanForward(bitboard);

          target_square = source_square - 8;

          if (!(target_square < a8) && !getbit(occboards[both], target_square))
          {
            if (source_square >= a7 && source_square <= h7)
            {
              addmove(possible_moves, encodemove(source_square, target_square, piece, Q, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, R, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, B, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, N, 0, 0, 0, 0));
            }

            else
            {
              addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));

              if ((source_square >= a2 && source_square <= h2) && !getbit(occboards[both], target_square - 8))
                addmove(possible_moves, encodemove(source_square, (target_square - 8), piece, 0, 0, 1, 0, 0));
            }
          }

          attacks = pawn_attacks[side][source_square] & occboards[black];

          while (attacks)
          {
            target_square = bitScanForward(attacks);

            if (source_square >= a7 && source_square <= h7)
            {
              addmove(possible_moves, encodemove(source_square, target_square, piece, Q, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, R, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, B, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, N, 1, 0, 0, 0));
            }

            else
              addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));

            popbit(attacks, target_square);
          }

          if (enpassant != null_square)
          {
            U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

            if (enpassant_attacks)
            {
              int target_enpassant = bitScanForward(enpassant_attacks);
              addmove(possible_moves, encodemove(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
            }
          }

          popbit(bitboard, source_square);
        }
      }

      if (piece == K)
      {
        if (castle & wk)
        {
          if (!getbit(occboards[both], f1) && !getbit(occboards[both], g1))
          {
            if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
              addmove(possible_moves, encodemove(e1, g1, piece, 0, 0, 0, 0, 1));
          }
        }

        if (castle & wq)
        {
          if (!getbit(occboards[both], d1) && !getbit(occboards[both], c1) && !getbit(occboards[both], b1))
          {
            if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black))
              addmove(possible_moves, encodemove(e1, c1, piece, 0, 0, 0, 0, 1));
          }
        }
      }
    }

    else
    {
      if (piece == p)
      {
        while (bitboard)
        {
          source_square = bitScanForward(bitboard);

          target_square = source_square + 8;

          if (!(target_square > h1) && !getbit(occboards[both], target_square))
          {
            if (source_square >= a2 && source_square <= h2)
            {
              addmove(possible_moves, encodemove(source_square, target_square, piece, q, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, r, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, b, 0, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, n, 0, 0, 0, 0));
            }

            else
            {
              addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));

              if ((source_square >= a7 && source_square <= h7) && !getbit(occboards[both], target_square + 8))
                addmove(possible_moves, encodemove(source_square, (target_square + 8), piece, 0, 0, 1, 0, 0));
            }
          }

          attacks = pawn_attacks[side][source_square] & occboards[white];

          while (attacks)
          {
            target_square = bitScanForward(attacks);

            if (source_square >= a2 && source_square <= h2)
            {
              addmove(possible_moves, encodemove(source_square, target_square, piece, q, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, r, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, b, 1, 0, 0, 0));
              addmove(possible_moves, encodemove(source_square, target_square, piece, n, 1, 0, 0, 0));
            }

            else
              addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));

            popbit(attacks, target_square);
          }

          if (enpassant != null_square)
          {
            U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

            if (enpassant_attacks)
            {
              int target_enpassant = bitScanForward(enpassant_attacks);
              addmove(possible_moves, encodemove(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
            }
          }

          popbit(bitboard, source_square);
        }
      }

      if (piece == k)
      {
        if (castle & bk)
        {
          if (!getbit(occboards[both], f8) && !getbit(occboards[both], g8))
          {
            if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
              addmove(possible_moves, encodemove(e8, g8, piece, 0, 0, 0, 0, 1));
          }
        }

        if (castle & bq)
        {
          if (!getbit(occboards[both], d8) && !getbit(occboards[both], c8) && !getbit(occboards[both], b8))
          {
            if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white))
              addmove(possible_moves, encodemove(e8, c8, piece, 0, 0, 0, 0, 1));
          }
        }
      }
    }

    if ((side == white) ? piece == N : piece == n)
    {
      while (bitboard)
      {
        source_square = bitScanForward(bitboard);

        attacks = knight_attacks[source_square] & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks)
        {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square))
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));

          else
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }

    if ((side == white) ? piece == B : piece == b)
    {
      while (bitboard)
      {
        source_square = bitScanForward(bitboard);

        attacks = get_bishop_attacks(source_square, occboards[both]) & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks)
        {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square))
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));

          else
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }

    if ((side == white) ? piece == R : piece == r)
    {
      while (bitboard)
      {
        source_square = bitScanForward(bitboard);

        attacks = get_rook_attacks(source_square, occboards[both]) & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks)
        {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square))
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));

          else
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }

    if ((side == white) ? piece == Q : piece == q)
    {
      while (bitboard)
      {
        source_square = bitScanForward(bitboard);

        attacks = get_queen_attacks(source_square, occboards[both]) & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks)
        {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square))
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));

          else
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }

    if ((side == white) ? piece == K : piece == k)
    {
      while (bitboard)
      {
        source_square = bitScanForward(bitboard);

        attacks = king_attacks[source_square] & ((side == white) ? ~occboards[white] : ~occboards[black]);

        while (attacks)
        {
          target_square = bitScanForward(attacks);

          if (!getbit(((side == white) ? occboards[black] : occboards[white]), target_square))
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 0, 0, 0, 0));

          else
            addmove(possible_moves, encodemove(source_square, target_square, piece, 0, 1, 0, 0, 0));

          popbit(attacks, target_square);
        }

        popbit(bitboard, source_square);
      }
    }
  }
}

static inline int Evaluate(int depth)
{
  if (depth == 0)
  {
    nodes++;
    return 6;
  }

  moves possible_moves[1];
  memset(possible_moves, 0, sizeof(possible_moves));
  Moves(possible_moves);

  for (int move_count = 0; move_count < possible_moves->count; move_count++)
  {
    copy_board();

    if (getmovesource(possible_moves->moves[move_count]) == getmovetarget(possible_moves->moves[move_count]))
    {
      continue;
    }
    int result = makemove(possible_moves->moves[move_count], all_moves);
    // print_board(0);
    if (result == 0)
    {
      take_back();
      continue;
    }

    if (result == 2)
    {
      possible_moves->win = 1;
      // if the player will win, we don't need to look at further moves..
      // winning_move = possible_moves->moves[move_count];
      return side;
    }

    if (result == stale)
    {
      // if the move results in stalemate..
      possible_moves->draw = 1;
      return stale;
    }

    // if none of the above it must be a normal move and we continue on
    if (possible_moves->cont == 0)
      possible_moves->cont = 1;
    else
      possible_moves->cont = 2;

    side = !side;
    result = Evaluate(depth - 1);
    take_back();

    // if move comes back as a win
    if (result == side)
    {
      possible_moves->win = 1;
      // possible_moves->winning_move = possible_moves->moves[move_count];
    }

    // if not then we check if == draw..
    else if (result == 5 || result == both)
    {
      possible_moves->draw = 1;
      // cbranch.count -= 1;
    }

    // any moves that aren't immediate draws / checkmates...
    else if (result == 6)
    {
      possible_moves->cont = 1;
      // cbranch.count -= 1;
    }

    // and if no wins, draws, or continue playings then we must lose :(
    else
    {
      possible_moves->lose = 1;
      if (possible_moves->cont > 0)
      {
        possible_moves->cont -= 1;
      }
      // possible_moves->winning_move = possible_moves->moves[move_count];
    }
  }

  // after the above has been done,  we check if we have any wins
  // if we do, we obviously want to play them, so we return this node with a win for the current side..
  if (possible_moves->win > 0)
  {
    // print_move(possible_moves->winning_move);
    return side;
  }
  // if their are continues
  if (possible_moves->cont > 0)
  {
    return contin;
  }
  // if no wins, and no continues we dont want opponent to win, so we return a draw if their are any..
  if (possible_moves->draw > 0)
  {
    return both;
  }
  // and if no wins draws or continues, but there are losses, then opponent must win :(
  if (possible_moves->lose > 0)
  {
    // print_move(winning_move);
    return !side;
  }

  // we would get to this point if there were no legal moves evaluated..
  // so either the position is already in checkmate
  // or the position is a stalemate.., so we check -> is it a checkmate by seeing if our our king is attacked
  if (is_square_attacked((side == white) ? bitScanForward(bitboards[K]) : bitScanForward(bitboards[k]), side))
    return !side;

  // and if not attacked then its a stalemate
  else
    return both;
}

moves get_moves(int source)
{
  moves possible_moves[1];
  memset(possible_moves, 0, sizeof(possible_moves));
  Moves(possible_moves);
  moves moves_to_return[1];
  memset(moves_to_return, 0, sizeof(moves_to_return));
  for (int move_count = 0; move_count < possible_moves->count; move_count++)
  {
    if (getmovesource(possible_moves->moves[move_count]) == source)
    {
      moves_to_return->moves[moves_to_return->count++] = possible_moves->moves[move_count];
    }
  }

  return *moves_to_return;
}

char *py_makemove(int move)
{
  makemove(move, all_moves);
  side = !side;
  gen_fen();
  return PYFEN;
}

#define FEN "2bqkbn1/2pppp2/np2N3/r3P1p1/p2N2B1/5Q2/PPPPKPP1/RNB2r2 w KQkq - 0 1"
int main()
{
  init_all();
  unsigned long long moves_looked_at = __LONG_LONG_MAX__;
  int start;
  int end;
  for (unsigned long long i = 1, n = moves_looked_at * 2; i < n; i++)
  {
    parse_fen(FEN_START);
    printf("%llu: ", i);

    start = get_time_ms();
    int result = Evaluate(i);
    end = get_time_ms();

    printf("(%i ms) ", (end - start));
    if (result == contin)
    {
      printf("Nothing yet..\n");
    }
    else if (result == black)
    {
      printf("Black wins :)\n");
      getchar();
      return 0;
    }
    else if (result == white)
    {
      printf("White wins :)\n");
      getchar();
      return 0;
    }
    else
    {
      printf("Draw :|\n");
      getchar();
      return 0;
    }
  }
  return 0;
}