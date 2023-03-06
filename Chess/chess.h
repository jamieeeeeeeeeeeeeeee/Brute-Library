#include <stdio.h>
#include <string.h>
#include <windows.h>

#ifdef __LONG_LONG_MAX__
#else
#define __LONG_LONG_MAX__ 9223372036854775807
#endif

#define U64 unsigned long long
#define contin 6
#define stale 5

#define setbit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define getbit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define popbit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

#define addmove(possible_moves, move) ((possible_moves)->moves[possible_moves->count++] = (move))
#define encodemove(source, target, piece, promoted, capture, double, enpassant, castling) \
    (source) |          \
    (target << 6) |     \
    (piece << 12) |     \
    (promoted << 16) |  \
    (capture << 20) |   \
    (double << 21) |    \
    (enpassant << 22) | \
    (castling << 23)    \

#define getmovesource(move) (move & 0x3f)
#define getmovetarget(move) ((move & 0xfc0) >> 6)
#define getmovepiece(move) ((move & 0xf000) >> 12)
#define getmovepromoted(move) ((move & 0xf0000) >> 16)
#define getmovecapture(move) (move & 0x100000)
#define getmovedouble(move) (move & 0x200000)
#define getmoveenpassant(move) (move & 0x400000)
#define getmovecastling(move) (move & 0x800000)

#define copy_board()                                                      \
    U64 bitboards_copy[12], occboards_copy[3];                          \
    int side_copy, enpassant_copy, castle_copy;                           \
    memcpy(bitboards_copy, bitboards, 96);                                \
    memcpy(occboards_copy, occboards, 24);                            \
    side_copy = side, enpassant_copy = enpassant, castle_copy = castle;   \

#define take_back()                                                       \
    memcpy(bitboards, bitboards_copy, 96);                                \
    memcpy(occboards, occboards_copy, 24);                            \
    side = side_copy, enpassant = enpassant_copy, castle = castle_copy;   \
    //if (cbranch.lastmovewasacapture == 1) {cbranch.lastcapture == cbranch.lastcapturecopy;} \
    //cbranch.lastmovewasacapture = 0;

#define FEN_START "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "

typedef struct {
  int moves[300];
  unsigned long long count;
  char win;
  char draw;
  char lose;
  char cont;
  int winning_move;
} moves;

typedef struct
{
  int leaves[1000000];
  int count;
  int lastcapture;
  int lastcapturecopy;
  int lastmovewasacapture;
} branch;

enum {
  a8, b8, c8, d8, e8, f8, g8, h8,
  a7, b7, c7, d7, e7, f7, g7, h7,
  a6, b6, c6, d6, e6, f6, g6, h6,
  a5, b5, c5, d5, e5, f5, g5, h5,
  a4, b4, c4, d4, e4, f4, g4, h4,
  a3, b3, c3, d3, e3, f3, g3, h3,
  a2, b2, c2, d2, e2, f2, g2, h2,
  a1, b1, c1, d1, e1, f1, g1, h1, null_square
};
enum {
  P, N, B, R, Q, K, p, n, b, r, q, k
};
enum {
  white, black, both
};
enum {
  rook, bishop
};
enum {
  wk = 1, wq = 2, bk = 4, bq = 8
};
enum {
  all_moves, matecheck
};

const char* square_to_coordinates[] = { "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};
const char ascii_pieces[12] = "PNBRQKpnbrqk";
const char* unicode_pieces[12] = { "♟︎", "♞", "♝", "♜", "♛", "♚", "♙", "♘", "♗", "♖", "q", "♔" };
const int char_pieces[] = {
    ['P'] = P,
    ['N'] = N,
    ['B'] = B,
    ['R'] = R,
    ['Q'] = Q,
    ['K'] = K,
    ['p'] = p,
    ['n'] = n,
    ['b'] = b,
    ['r'] = r,
    ['q'] = q,
    ['k'] = k
};
const char promoted_pieces[] = {
    [Q] = 'q',
    [R] = 'r',
    [B] = 'b',
    [N] = 'n',
    [q] = 'q',
    [r] = 'r',
    [b] = 'b',
    [n] = 'n'
};
const U64 NOTAFILE = 18374403900871474942ULL;
const U64 NOTHFILE = 9187201950435737471ULL;
const U64 NOTHGFILE = 4557430888798830399ULL;
const U64 NOTABFILE = 18229723555195321596ULL;
const int castling_rights[64] = {
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};

const U64 rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL
};
const U64 bishop_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL
};
const int bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};
const int rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

const int lsb_64_table[64] =
{
   63, 30,  3, 32, 59, 14, 11, 33,
   60, 24, 50,  9, 55, 19, 21, 34,
   61, 29,  2, 53, 51, 23, 41, 18,
   56, 28,  1, 43, 46, 27,  0, 35,
   62, 31, 58,  4,  5, 49, 54,  6,
   15, 52, 12, 40,  7, 42, 45, 16,
   25, 57, 48, 13, 10, 39,  8, 44,
   20, 47, 38, 22, 17, 37, 36, 26
};


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

// Bit fiddle functions
unsigned int folded;
/**
 * bitScanForward
 * @author Matt Taylor (2003)
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
int bitScanForward(U64 bb) {
  bb ^= bb - 1;
  folded = (int)bb ^ (bb >> 32);
  return lsb_64_table[folded * 0x78291ACF >> 26];
}
static int count_bits(U64 bitboard) {
#ifdef __builtin_popcountll
  return __builtin_popcountll(bitboard);
#else
  // Use your own popcount function if the built-in function is not available
  int count = 0;
  while (bitboard) {
    count++;
    bitboard &= bitboard - 1;
  }
  return count;
#endif
}

// Board representation helpers 
void print_bitboard(U64 bitboard) {
  printf("\n");

  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;

      if (!file) {
        printf("  %d ", 8 - rank);
      }
      printf(" %d", getbit(bitboard, square) ? 1 : 0);
    }
    printf("\n");
  }

  printf("\n     a b c d e f g h\n\n");
  printf("     Bitboard: %llud\n\n", bitboard);
}
void print_board(char info) {
  printf("\n");

  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;
      if (!file) {
        printf("  %d ", 8 - rank);
      }
      int piece = -1;

      for (int bb_piece = P; bb_piece <= k; bb_piece++) {
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

  if (info) {
    printf("     Side:     %s\n", !side ? "white" : "black");
    printf("     Enpassant:   %s\n", (enpassant != null_square) ? square_to_coordinates[enpassant] : "no");
    printf("     Castling:  %c%c%c%c\n\n", (castle & wk) ? 'K' : '-',
      (castle & wq) ? 'Q' : '-',
      (castle & bk) ? 'k' : '-',
      (castle & bq) ? 'q' : '-');
  }
}
U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask) {
  U64 occupancy = 0ULL;

  for (int count = 0; count < bits_in_mask; count++) {
    int square = bitScanForward(attack_mask);
    popbit(attack_mask, square);

    if (index & (1 << count)) {
      occupancy |= (1ULL << square);
    }
  }

  return occupancy;
}
int get_time_ms(void) {
  // #ifdef WIN64
  return GetTickCount();
  // #else
  //     struct timeval time_value;
  //     gettimeofday(&time_value, NULL);
  //     return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
  // #endif
}
void parse_fen(char* fen) {
  memset(bitboards, 0ULL, sizeof(bitboards));
  memset(occboards, 0ULL, sizeof(occboards));

  side = 0;
  enpassant = null_square;
  castle = 0;

  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;

      if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
        int piece = char_pieces[*fen];
        setbit(bitboards[piece], square);

        fen++;
      }

      if (*fen >= '0' && *fen <= '9') {
        int offset = *fen - '0';
        int piece = -1;

        for (int bb_piece = P; bb_piece <= k; bb_piece++) {
          if (getbit(bitboards[bb_piece], square))
            piece = bb_piece;
        }

        if (piece == -1) {
          file--;
        }

        file += offset;

        fen++;
      }

      if (*fen == '/') {
        fen++;
      }
    }
  }

  fen++;

  (*fen == 'w') ? (side = white) : (side = black);

  fen += 2;

  while (*fen != ' ') {
    switch (*fen) {
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

  if (*fen != '-') {
    int file = fen[0] - 'a';
    int rank = 8 - (fen[1] - '0');

    enpassant = rank * 8 + file;
  }
  else {
    enpassant = null_square;
  }

  for (int piece = P; piece <= K; piece++) {
    occboards[white] |= bitboards[piece];
  }

  for (int piece = p; piece <= k; piece++) {
    occboards[black] |= bitboards[piece];
  }

  occboards[both] |= occboards[white];
  occboards[both] |= occboards[black];
  print_board(1);
}

// Fill out the attack tables
U64 mask_pawn_attacks(int side, int square) {
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  setbit(bitboard, square);

  if (!side) {
    if ((bitboard >> 7) & NOTAFILE) {
      attacks |= (bitboard >> 7);
    }
    if ((bitboard >> 9) & NOTHFILE) {
      attacks |= (bitboard >> 9);
    }
  }
  else {
    if ((bitboard << 7) & NOTHFILE) {
      attacks |= (bitboard << 7);
    }
    if ((bitboard << 9) & NOTAFILE) {
      attacks |= (bitboard << 9);
    }
  }

  return attacks;
}
U64 mask_knight_attacks(int square) {
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  setbit(bitboard, square);

  if ((bitboard >> 17) & NOTHFILE) {
    attacks |= (bitboard >> 17);
  }
  if ((bitboard >> 15) & NOTAFILE) {
    attacks |= (bitboard >> 15);
  }
  if ((bitboard >> 10) & NOTHGFILE) {
    attacks |= (bitboard >> 10);
  }
  if ((bitboard >> 6) & NOTABFILE) {
    attacks |= (bitboard >> 6);
  }
  if ((bitboard << 17) & NOTAFILE) {
    attacks |= (bitboard << 17);
  }
  if ((bitboard << 15) & NOTHFILE) {
    attacks |= (bitboard << 15);
  }
  if ((bitboard << 10) & NOTABFILE) {
    attacks |= (bitboard << 10);
  }
  if ((bitboard << 6) & NOTHGFILE) {
    attacks |= (bitboard << 6);
  }

  return attacks;
}
U64 mask_king_attacks(int square) {
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  setbit(bitboard, square);

  if (bitboard >> 8) {
    attacks |= (bitboard >> 8);
  }
  if ((bitboard >> 9) & NOTHFILE) {
    attacks |= (bitboard >> 9);
  }
  if ((bitboard >> 7) & NOTAFILE) {
    attacks |= (bitboard >> 7);
  }
  if ((bitboard >> 1) & NOTHFILE) {
    attacks |= (bitboard >> 1);
  }
  if (bitboard << 8) {
    attacks |= (bitboard << 8);
  }
  if ((bitboard << 9) & NOTAFILE) {
    attacks |= (bitboard << 9);
  }
  if ((bitboard << 7) & NOTHFILE) {
    attacks |= (bitboard << 7);
  }
  if ((bitboard << 1) & NOTAFILE) {
    attacks |= (bitboard << 1);
  }

  return attacks;
}
U64 mask_bishop_attacks(int square) {
  U64 attacks = 0ULL;

  int r, f;
  int tr = square / 8;
  int tf = square % 8;

  for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) {
    attacks |= (1ULL << (r * 8 + f));
  }
  for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) {
    attacks |= (1ULL << (r * 8 + f));
  }
  for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) {
    attacks |= (1ULL << (r * 8 + f));
  }
  for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) {
    attacks |= (1ULL << (r * 8 + f));
  }

  return attacks;
}
U64 mask_rook_attacks(int square) {
  U64 attacks = 0ULL;

  int r, f;

  int tr = square / 8;
  int tf = square % 8;

  for (r = tr + 1; r <= 6; r++) {
    attacks |= (1ULL << (r * 8 + tf));
  }
  for (r = tr - 1; r >= 1; r--) {
    attacks |= (1ULL << (r * 8 + tf));
  }
  for (f = tf + 1; f <= 6; f++) {
    attacks |= (1ULL << (tr * 8 + f));
  }
  for (f = tf - 1; f >= 1; f--) {
    attacks |= (1ULL << (tr * 8 + f));
  }

  return attacks;
}

// Move gen helpers
static U64 get_bishop_attacks(int square, U64 occupancy) {
  occupancy &= bishop_masks[square];
  occupancy *= bishop_magic_numbers[square];
  occupancy >>= 64 - bishop_relevant_bits[square];

  return bishop_attacks[square][occupancy];
}
static U64 get_rook_attacks(int square, U64 occupancy) {
  occupancy &= rook_masks[square];
  occupancy *= rook_magic_numbers[square];
  occupancy >>= 64 - rook_relevant_bits[square];

  return rook_attacks[square][occupancy];
}
static U64 get_queen_attacks(int square, U64 occupancy) {
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
static int is_square_attacked(int square, int side) {
  if ((side == white) && (pawn_attacks[black][square] & bitboards[P])) {
    return 1;
  }
  if ((side == black) && (pawn_attacks[white][square] & bitboards[p])) {
    return 1;
  }
  if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) {
    ;
    return 1;
  }
  if (get_bishop_attacks(square, occboards[both]) & ((side == white) ? bitboards[B] : bitboards[b])) {
    return 1;
  }
  if (get_rook_attacks(square, occboards[both]) & ((side == white) ? bitboards[R] : bitboards[r])) {
    return 1;
  }
  if (get_queen_attacks(square, occboards[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) {
    return 1;
  }
  return (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k]));
}
static int makemove(int move, int move_flag) {
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

  if (capture) {
    // print_board(1);
    // getchar();
    int start_piece, end_piece;

    if (side == white) {
      start_piece = p;
      end_piece = k;
    }
    else {
      start_piece = P;
      end_piece = K;
    }

    for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++) {
      if (getbit(bitboards[bb_piece], target_square)) {
        popbit(bitboards[bb_piece], target_square);
        break;
      }
    }
  }

  if (promoted_piece) {
    popbit(bitboards[(side == white) ? P : p], target_square);
    setbit(bitboards[promoted_piece], target_square);
  }

  if (enpass) {
    (side == white) ? popbit(bitboards[p], target_square + 8) : popbit(bitboards[P], target_square - 8);
  }

  enpassant = null_square;

  if (double_push) {
    (side == white) ? (enpassant = target_square + 8) : (enpassant = target_square - 8);
  }

  if (castling) {
    switch (target_square) {
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

  for (int bb_piece = P; bb_piece <= K; bb_piece++) {
    occboards[white] |= bitboards[bb_piece];
  }

  for (int bb_piece = p; bb_piece <= k; bb_piece++) {
    occboards[black] |= bitboards[bb_piece];
  }

  occboards[both] |= occboards[white];
  occboards[both] |= occboards[black];

  /*if (move_flag == all_moves) {
      print_board(0);
      getchar();
  }*/
  // print_board(0);

  if (is_square_attacked((side == white) ? bitScanForward(bitboards[K]) : bitScanForward(bitboards[k]), !side)) {
    return 0;
  }

  if (move_flag == all_moves) {

    // print_move(move);
    /*getchar();
    if (getmovetarget(move) == f6)
        printf("stop\n");*/

    checkmateresult = Evaluate();
    if (checkmateresult == 'c') {
      // print_board(0);
      // printf("Checkmate with ");
      // print_move(move);
      // getchar();
      return 2;
    }
    else if (checkmateresult == 's') {
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

// Debug functions
void print_move(int move) {
  if (getmovepromoted(move)) {
    printf("%s%s%c\n", unicode_pieces[getmovepiece(move)],
      square_to_coordinates[getmovetarget(move)],
      promoted_pieces[getmovepromoted(move)]);
  }
  else {
    printf("%s%s\n", unicode_pieces[getmovepiece(move)],
      square_to_coordinates[getmovetarget(move)]);
  }
}
void print_possible_moves(moves* possible_moves) {
  if (!possible_moves->count) {
    printf("\n     No move in the move list!\n");
    return;
  }

  printf("\n     move    piece     capture   double    enpass    castling\n\n");

  for (int move_count = 0; move_count < possible_moves->count; move_count++) {
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

// Python functions
void py_gen_fen(void) {
  int index = 0;

  for (int rank = 0; rank < 8; rank++) {
    int empty = 0;

    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;
      int piece = -1;

      for (int bb_piece = P; bb_piece <= k; bb_piece++) {
        if (getbit(bitboards[bb_piece], square))
          piece = bb_piece;
      }

      if (piece == -1) {
        empty++;
      }
      else {
        if (empty) {
          PYFEN[index++] = '0' + empty;
          empty = 0;
        }

        PYFEN[index++] = ascii_pieces[piece];
      }
    }

    if (empty) {
      PYFEN[index++] = '0' + empty;
    }

    if (rank != 7) {
      PYFEN[index++] = '/';
    }
  }

  PYFEN[index++] = ' ';
  PYFEN[index++] = (side == white) ? 'w' : 'b';
  PYFEN[index++] = ' ';

  if (castle & wk) {
    PYFEN[index++] = 'K';
  }

  if (castle & wq) {
    PYFEN[index++] = 'Q';
  }

  if (castle & bk) {
    PYFEN[index++] = 'k';
  }

  if (castle & bq) {
    PYFEN[index++] = 'q';
  }

  if (!(castle & wk) && !(castle & wq) && !(castle & bk) && !(castle & bq)) {
    PYFEN[index++] = '-';
  }

  PYFEN[index++] = ' ';

  if (enpassant != null_square) {
    PYFEN[index++] = square_to_coordinates[enpassant][0];
    PYFEN[index++] = square_to_coordinates[enpassant][1];
  }
  else {
    PYFEN[index++] = '-';
  }

  PYFEN[index] = '\0';
  printf("%s", PYFEN);
}
char* py_makemove(int move) {
  makemove(move, all_moves);
  side = !side;
  py_gen_fen();
  return PYFEN;
}
