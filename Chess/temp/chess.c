//#include "chess.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef WIN32
#include <windows.h>
#else 
# include <sys/time.h>
#endif

#define version " - exhaustive search - 1.0.0"
#define U64 unsigned long long

// FEN debug positions
#define empty_board "8/8/8/8/8/8/8/8 b - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "

// Board representation and manipulation globals
enum {
  a8, b8, c8, d8, e8, f8, g8, h8,
  a7, b7, c7, d7, e7, f7, g7, h7,
  a6, b6, c6, d6, e6, f6, g6, h6,
  a5, b5, c5, d5, e5, f5, g5, h5,
  a4, b4, c4, d4, e4, f4, g4, h4,
  a3, b3, c3, d3, e3, f3, g3, h3,
  a2, b2, c2, d2, e2, f2, g2, h2,
  a1, b1, c1, d1, e1, f1, g1, h1, no_sq
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

const char* square_to_coordinates[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};
char ascii_pieces[12] = "PNBRQKpnbrqk";
char* unicode_pieces[12] = { "♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚" };
int char_pieces[] = {
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
char promoted_pieces[] = {
    [Q] = 'q',
    [R] = 'r',
    [B] = 'b',
    [N] = 'n',
    [q] = 'q',
    [r] = 'r',
    [b] = 'b',
    [n] = 'n'
};

U64 bitboards[12];
U64 occupancies[3];
int side;
int enpassant = no_sq;
int castle;
U64 hash_key;
U64 repetition_table[1000];  // 1000 is a number of plies (500 moves) in the entire game
int repetition_index;
int ply;
int fifty;

#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))
static inline int count_bits(U64 bitboard) {
  // bit counter
  int count = 0;

  // consecutively reset least significant 1st bit
  while (bitboard) {
    // increment count
    count++;

    // reset least significant 1st bit
    bitboard &= bitboard - 1;
  }

  // return bit count
  return count;
}
static inline int get_ls1b_index(U64 bitboard) {
  // make sure bitboard is not 0
  if (bitboard) {
    // count trailing bits before LS1B
    return count_bits((bitboard & -bitboard) - 1);
  }

  //otherwise
  else
    // return illegal index
    return -1;
}

// UCI globals
int quit = 0;
int movestogo = 30;
int movetime = -1;
int time = -1;
int inc = 0;
int starttime = 0;
int stoptime = 0;
int timeset = 0;
int stopped = 0;

// Random number generator
unsigned int random_state = 1804289383;
unsigned int get_random_U32_number() {
  // get current state
  unsigned int number = random_state;

  // XOR shift algorithm
  number ^= number << 13;
  number ^= number >> 17;
  number ^= number << 5;

  // update random number state
  random_state = number;

  // return random number
  return number;
}
U64 get_random_U64_number() {
  // define 4 random numbers
  U64 n1, n2, n3, n4;

  // init random numbers slicing 16 bits from MS1B side
  n1 = (U64)(get_random_U32_number()) & 0xFFFF;
  n2 = (U64)(get_random_U32_number()) & 0xFFFF;
  n3 = (U64)(get_random_U32_number()) & 0xFFFF;
  n4 = (U64)(get_random_U32_number()) & 0xFFFF;

  // return random number
  return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}
U64 generate_magic_number() {
  return get_random_U64_number() & get_random_U64_number() & get_random_U64_number();
}

// Zobrist keys
U64 piece_keys[12][64];
U64 enpassant_keys[64];
U64 castle_keys[16];
U64 side_key;
void init_random_keys() {
  // update pseudo random number state
  random_state = 1804289383;

  // loop over piece codes
  for (int piece = P; piece <= k; piece++) {
    // loop over board squares
    for (int square = 0; square < 64; square++)
      // init random piece keys
      piece_keys[piece][square] = get_random_U64_number();
  }

  // loop over board squares
  for (int square = 0; square < 64; square++)
    // init random enpassant keys
    enpassant_keys[square] = get_random_U64_number();

  // loop over castling keys
  for (int index = 0; index < 16; index++)
    // init castling keys
    castle_keys[index] = get_random_U64_number();

  // init random side key
  side_key = get_random_U64_number();
}
U64 generate_hash_key() {
  // final hash key
  U64 final_key = 0ULL;

  // temp piece bitboard copy
  U64 bitboard;

  // loop over piece bitboards
  for (int piece = P; piece <= k; piece++) {
    // init piece bitboard copy
    bitboard = bitboards[piece];

    // loop over the pieces within a bitboard
    while (bitboard) {
      // init square occupied by the piece
      int square = get_ls1b_index(bitboard);

      // hash piece
      final_key ^= piece_keys[piece][square];

      // pop LS1B
      pop_bit(bitboard, square);
    }
  }

  // if enpassant square is on board
  if (enpassant != no_sq)
    // hash enpassant
    final_key ^= enpassant_keys[enpassant];

  // hash castling rights
  final_key ^= castle_keys[castle];

  // hash the side only if black is to move
  if (side == black) final_key ^= side_key;

  // return generated hash key
  return final_key;
}

// I/O 
void print_bitboard(U64 bitboard) {
  // print offset
  printf("\n");

  // loop over board ranks
  for (int rank = 0; rank < 8; rank++) {
    // loop over board files
    for (int file = 0; file < 8; file++) {
      // convert file & rank into square index
      int square = rank * 8 + file;

      // print ranks
      if (!file)
        printf("  %d ", 8 - rank);

      // print bit state (either 1 or 0)
      printf(" %d", get_bit(bitboard, square) ? 1 : 0);

    }

    // print new line every rank
    printf("\n");
  }

  // print board files
  printf("\n     a b c d e f g h\n\n");

  // print bitboard as unsigned decimal number
  printf("     Bitboard: %llud\n\n", bitboard);
}
void print_board() {
  // print offset
  printf("\n");

  // loop over board ranks
  for (int rank = 0; rank < 8; rank++) {
    // loop ober board files
    for (int file = 0; file < 8; file++) {
      // init square
      int square = rank * 8 + file;

      // print ranks
      if (!file)
        printf("  %d ", 8 - rank);

      // define piece variable
      int piece = -1;

      // loop over all piece bitboards
      for (int bb_piece = P; bb_piece <= k; bb_piece++) {
        // if there is a piece on current square
        if (get_bit(bitboards[bb_piece], square))
          // get piece code
          piece = bb_piece;
      }

      // print different piece set depending on OS
#ifdef WIN64
      printf(" %c", (piece == -1) ? '.' : ascii_pieces[piece]);
#else
      printf(" %s", (piece == -1) ? "." : unicode_pieces[piece]);
#endif
    }

    // print new line every rank
    printf("\n");
  }

  // print board files
  printf("\n     a b c d e f g h\n\n");

  // print side to move
  printf("     Side:     %s\n", !side ? "white" : "black");

  // print enpassant square
  printf("     Enpassant:   %s\n", (enpassant != no_sq) ? square_to_coordinates[enpassant] : "no");

  // print castling rights
  printf("     Castling:  %c%c%c%c\n\n", (castle & wk) ? 'K' : '-',
    (castle & wq) ? 'Q' : '-',
    (castle & bk) ? 'k' : '-',
    (castle & bq) ? 'q' : '-');

  // print hash key
  printf("     Hash key:  %llx\n", hash_key);

  // fifty move rule counter
  printf("     Fifty move: %d\n\n", fifty);
}
void reset_board() {
  // reset board position (bitboards)
  memset(bitboards, 0ULL, sizeof(bitboards));

  // reset occupancies (bitboards)
  memset(occupancies, 0ULL, sizeof(occupancies));

  // reset game state variables
  side = 0;
  enpassant = no_sq;
  castle = 0;

  // reset repetition index
  repetition_index = 0;

  // reset fifty move rule counter
  fifty = 0;

  // reset repetition table
  memset(repetition_table, 0ULL, sizeof(repetition_table));
}
void parse_fen(char* fen) {
  // prepare for new game
  reset_board();

  // loop over board ranks
  for (int rank = 0; rank < 8; rank++) {
    // loop over board files
    for (int file = 0; file < 8; file++) {
      // init current square
      int square = rank * 8 + file;

      // match ascii pieces within FEN string
      if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
        // init piece type
        int piece = char_pieces[*fen];

        // set piece on corresponding bitboard
        set_bit(bitboards[piece], square);

        // increment pointer to FEN string
        fen++;
      }

      // match empty square numbers within FEN string
      if (*fen >= '0' && *fen <= '9') {
        // init offset (convert char 0 to int 0)
        int offset = *fen - '0';

        // define piece variable
        int piece = -1;

        // loop over all piece bitboards
        for (int bb_piece = P; bb_piece <= k; bb_piece++) {
          // if there is a piece on current square
          if (get_bit(bitboards[bb_piece], square))
            // get piece code
            piece = bb_piece;
        }

        // on empty current square
        if (piece == -1)
          // decrement file
          file--;

        // adjust file counter
        file += offset;

        // increment pointer to FEN string
        fen++;
      }

      // match rank separator
      if (*fen == '/')
        // increment pointer to FEN string
        fen++;
    }
  }

  // got to parsing side to move (increment pointer to FEN string)
  fen++;

  // parse side to move
  (*fen == 'w') ? (side = white) : (side = black);

  // go to parsing castling rights
  fen += 2;

  // parse castling rights
  while (*fen != ' ') {
    switch (*fen) {
    case 'K': castle |= wk; break;
    case 'Q': castle |= wq; break;
    case 'k': castle |= bk; break;
    case 'q': castle |= bq; break;
    case '-': break;
    }

    // increment pointer to FEN string
    fen++;
  }

  // go to parsing enpassant square (increment pointer to FEN string)
  fen++;

  // parse enpassant square
  if (*fen != '-') {
    // parse enpassant file & rank
    int file = fen[0] - 'a';
    int rank = 8 - (fen[1] - '0');

    // init enpassant square
    enpassant = rank * 8 + file;
  }

  // no enpassant square
  else
    enpassant = no_sq;

  // go to parsing half move counter (increment pointer to FEN string)
  fen++;

  // parse half move counter to init fifty move counter
  fifty = atoi(fen);

  // loop over white pieces bitboards
  for (int piece = P; piece <= K; piece++)
    // populate white occupancy bitboard
    occupancies[white] |= bitboards[piece];

  // loop over black pieces bitboards
  for (int piece = p; piece <= k; piece++)
    // populate white occupancy bitboard
    occupancies[black] |= bitboards[piece];

  // init all occupancies
  occupancies[both] |= occupancies[white];
  occupancies[both] |= occupancies[black];

  // init hash key
  hash_key = generate_hash_key();
}
int input_waiting() {
#ifndef WIN32
  fd_set readfds;
  struct timeval tv;
  FD_ZERO(&readfds);
  FD_SET(fileno(stdin), &readfds);
  tv.tv_sec = 0; tv.tv_usec = 0;
  select(16, &readfds, 0, 0, &tv);

  return (FD_ISSET(fileno(stdin), &readfds));
#else
  static int init = 0, pipe;
  static HANDLE inh;
  DWORD dw;

  if (!init) {
    init = 1;
    inh = GetStdHandle(STD_INPUT_HANDLE);
    pipe = !GetConsoleMode(inh, &dw);
    if (!pipe) {
      SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
      FlushConsoleInputBuffer(inh);
    }
  }

  if (pipe) {
    if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
    return dw;
  }

  else {
    GetNumberOfConsoleInputEvents(inh, &dw);
    return dw <= 1 ? 0 : dw;
  }

#endif
}
void read_input() {
  // bytes to read holder
  int bytes;

  // GUI/user input
  char input[256] = "", * endc;

  // "listen" to STDIN
  if (input_waiting()) {
    // tell engine to stop calculating
    stopped = 1;

    // loop to read bytes from STDIN
    do {
      // read bytes from STDIN
      bytes = read(fileno(stdin), input, 256);
    }

    // until bytes available
    while (bytes < 0);

    // searches for the first occurrence of '\n'
    endc = strchr(input, '\n');

    // if found new line set value at pointer to 0
    if (endc) *endc = 0;

    // if input is available
    if (strlen(input) > 0) {
      // match UCI "quit" command
      if (!strncmp(input, "quit", 4))
        // tell engine to terminate exacution    
        quit = 1;

      // // match UCI "stop" command
      else if (!strncmp(input, "stop", 4))
        // tell engine to terminate exacution
        quit = 1;
    }
  }
}
static void communicate() {
  // if time is up break here
  if (timeset == 1 && get_time_ms() > stoptime) {
    // tell engine to stop calculating
    stopped = 1;
  }

  // read GUI input
  read_input();
}

// Attacks
const U64 not_a_file = 18374403900871474942ULL;
const U64 not_h_file = 9187201950435737471ULL;
const U64 not_hg_file = 4557430888798830399ULL;
const U64 not_ab_file = 18229723555195321596ULL;
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
void gen_fen(void) {
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
U64 bishop_attacks_on_the_fly(int square, U64 block) {
  U64 attacks = 0ULL;

  int r, f;
  int tr = square / 8;
  int tf = square % 8;

  for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
    attacks |= (1ULL << (r * 8 + f));
    if ((1ULL << (r * 8 + f)) & block) {
      break;
    }
  }

  for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
    attacks |= (1ULL << (r * 8 + f));
    if ((1ULL << (r * 8 + f)) & block) {
      break;
    }
  }

  for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) {
    attacks |= (1ULL << (r * 8 + f));
    if ((1ULL << (r * 8 + f)) & block) {
      break;
    }
  }

  for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
    attacks |= (1ULL << (r * 8 + f));
    if ((1ULL << (r * 8 + f)) & block)     {
      break;
    }
  }

  return attacks;
}
U64 rook_attacks_on_the_fly(int square, U64 block) {
  U64 attacks = 0ULL;

  int r, f;
  int tr = square / 8;
  int tf = square % 8;

  for (r = tr + 1; r <= 7; r++) {
    attacks |= (1ULL << (r * 8 + tf));
    if ((1ULL << (r * 8 + tf)) & block) {
      break;
    }
  }

  for (r = tr - 1; r >= 0; r--) {
    attacks |= (1ULL << (r * 8 + tf));
    if ((1ULL << (r * 8 + tf)) & block) {
      break;
    }
  }

  for (f = tf + 1; f <= 7; f++) {
    attacks |= (1ULL << (tr * 8 + f));
    if ((1ULL << (tr * 8 + f)) & block) {
      break;
    }
  }

  for (f = tf - 1; f >= 0; f--) {
    attacks |= (1ULL << (tr * 8 + f));
    if ((1ULL << (tr * 8 + f)) & block) {
      break;
    }
  }

  return attacks;
}
void init_leapers_attacks(void) {
  for (int square = 0; square < 64; square++) {
    pawn_attacks[white][square] = mask_pawn_attacks(white, square);
    pawn_attacks[black][square] = mask_pawn_attacks(black, square);
    knight_attacks[square] = mask_knight_attacks(square);
    king_attacks[square] = mask_king_attacks(square);
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
void init_sliders_attacks(int bishop) {
  for (int square = 0; square < 64; square++) {
    bishop_masks[square] = mask_bishop_attacks(square);
    rook_masks[square] = mask_rook_attacks(square);

    U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];
    int relevant_bits_count = count_bits(attack_mask);
    int occupancy_indicies = (1 << relevant_bits_count);

    for (int index = 0; index < occupancy_indicies; index++) {
      // bishop
      if (bishop) {
        U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
        int magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]);
        bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
      }
      else {
        U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
        int magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);
        rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
      }
    }
  }
}
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
int get_time_ms() {
#ifdef WIN64
  return GetTickCount();
#else
  struct timeval time_value;
  gettimeofday(&time_value, NULL);
  return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
#endif
}


// Move gen
static inline int is_square_attacked(int square, int side) {
  // attacked by white pawns
  if ((side == white) && (pawn_attacks[black][square] & bitboards[P])) return 1;

  // attacked by black pawns
  if ((side == black) && (pawn_attacks[white][square] & bitboards[p])) return 1;

  // attacked by knights
  if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) return 1;

  // attacked by bishops
  if (get_bishop_attacks(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b])) return 1;

  // attacked by rooks
  if (get_rook_attacks(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r])) return 1;

  // attacked by bishops
  if (get_queen_attacks(square, occupancies[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) return 1;

  // attacked by kings
  if (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) return 1;

  // by default return false
  return 0;
}
void print_attacked_squares(int side) {
  printf("\n");

  // loop over board ranks
  for (int rank = 0; rank < 8; rank++) {
    // loop over board files
    for (int file = 0; file < 8; file++) {
      // init square
      int square = rank * 8 + file;

      // print ranks
      if (!file)
        printf("  %d ", 8 - rank);

      // check whether current square is attacked or not
      printf(" %d", is_square_attacked(square, side) ? 1 : 0);
    }

    // print new line every rank
    printf("\n");
  }

  // print files
  printf("\n     a b c d e f g h\n\n");
}
#define encode_move(source, target, piece, promoted, capture, double, enpassant, castling) \
    (source) |          \
    (target << 6) |     \
    (piece << 12) |     \
    (promoted << 16) |  \
    (capture << 20) |   \
    (double << 21) |    \
    (enpassant << 22) | \
    (castling << 23)    \

#define get_move_source(move) (move & 0x3f)
#define get_move_target(move) ((move & 0xfc0) >> 6)
#define get_move_piece(move) ((move & 0xf000) >> 12)
#define get_move_promoted(move) ((move & 0xf0000) >> 16)
#define get_move_capture(move) (move & 0x100000)
#define get_move_double(move) (move & 0x200000)
#define get_move_enpassant(move) (move & 0x400000)
#define get_move_castling(move) (move & 0x800000)

// move list structure
typedef struct {
  // moves
  int moves[256];
  int count;
  char result; // 'w' - win, 'l' - lose, 'd' - draw, 'n' - no result
  int winning_move; // index of winning move
} moves;


void init_all(void) {
  init_leapers_attacks();

  init_sliders_attacks(bishop);
  init_sliders_attacks(rook);

  parse_fen(FEN_START);
  // init_magic_numbers();
}

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
      // print_board(0);
      //printf("Stalemate with");
      //print_move(move);
      return 5;
    }
  }
  return 1;
}
moves get_moves(int source) {
  moves possible_moves[1];
  memset(possible_moves, 0, sizeof(possible_moves));
  Moves(possible_moves);
  moves moves_to_return[1];
  memset(moves_to_return, 0, sizeof(moves_to_return));
  for (int move_count = 0; move_count < possible_moves->count; move_count++) {
    if (getmovesource(possible_moves->moves[move_count]) == source) {
      moves_to_return->moves[moves_to_return->count++] = possible_moves->moves[move_count];
    }
  }

  return *moves_to_return;
}
char* py_makemove(int move) {
  makemove(move, all_moves);
  side = !side;
  gen_fen();
  return PYFEN;
}

static void Moves(moves* possible_moves) {
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

/*
static inline int evaluate(void) {
  side = !side;
  moves posas[1];
  generate_moves(posas);

  side = !side;
  for (int move_count = 0; move_count < posas->count; move_count++) {
    copy_board();
    side = !side;
    if (!make_move(posas->moves[move_count], only_captures)) {
      take_back();
      continue;
    }
    take_back();
    return 0;
  }

  if (is_square_attacked((side == white) ? bitScanForward(bitboards[K]) : bitScanForward(bitboards[k]), side)) {
    return 1;
  }
  return 0;
}*/
static int Solve(int depth) {
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
#define FEN2 "6r1/p3p1rk/1p1pPp1p/q3n2R/4P3/3BR2P/PPP2QP1/7K w - - 0 1"
int main(int argc, char** argv) {
  init_all();
  unsigned long long moves_looked_at = __LONG_LONG_MAX__;
  int start;
  int end;
  for (unsigned long long i = 1, n = moves_looked_at * 2; i < n; i++) {
    parse_fen(FEN2);
    printf("%llu: ", i);

    start = get_time_ms();
    int result = Solve(i);
    end = get_time_ms();

    printf("(%i ms) ", (end - start));
    if (result == contin) {
      printf("Nothing yet..\n");
    }
    else if (result == black) {
      printf("Black wins :)\n");
      getchar();
      return 0;
    }
    else if (result == white) {
      printf("White wins :)\n");
      getchar();
      return 0;
    }
    else {
      printf("Draw :|\n");
      getchar();
      return 0;
    }
  }
  return 0;
}
