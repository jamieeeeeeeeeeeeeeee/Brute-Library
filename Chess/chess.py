import tkinter as tk
import tkinter.ttk as ttk
import sv_ttk

from ctypes import *

FOLDER = "./"
DEFAULT_BOARD = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

TEMP_NOTATION = {1:'a', 2:'b', 3:'c', 4:'d', 5:'e', 6:'f', 7:'g', 8:'h'}
POS = {(9):'K', (14):'Q', (13):'R', (12):'B', (11):'N', (10):'P', 0:'-', (1):'k', (6):'q', (5):'r', (4):'b', (3):'n', (2):'p'}
FEN = {'K':(9), 'Q':(14), 'R':(13), 'B':(12), 'N':(11), 'P':(10), '-':0, 'k':(1), 'q':(6), 'r':(5), 'b':(4), 'n':(3), 'p':(2)}
ALGEBRAIC = {-1:'-'}
LETTERS_TO_NUM = {}
for s in range(64):
        x = s % 8 
        y = s // 8
        letters = TEMP_NOTATION[x+1]+str(8-y)
        ALGEBRAIC[s] = letters
        LETTERS_TO_NUM[letters] = s

class Possible_Moves(Structure):
    _fields_ = [("moves", c_int * 300), ("count", c_ulonglong), ("win", c_char), ("draw", c_char), ("loss", c_char), ("cont", c_char), ("winning_move", c_int)]

try:
    #windows/linux
    c_functions = CDLL("./chess.so")
except:
    #mac
    c_functions = CDLL("./chess.dylib")

c_functions.init_all()
c_functions.get_moves.restype = Possible_Moves
c_functions.py_makemove.restype = c_char_p
class Piece: 
    blank = 0
    king = 1
    pawn = 2
    night = 3
    bishop = 4
    rook = 5
    queen = 6

    white = 7
    black = 8

    def is_colour(piece, colour=white): #By default this will check if a piece is white
        bit = piece.bit_length() 

        if bit == 4:
            if colour == Piece.white:
                return True
            return False
        elif bit < 4 and colour == Piece.white:
            return False
        return True

def fen_to_board(fen):
    info = False

    pos = [0]*64
    index = 0

    for i in list(fen):
        if info:
            break
        else:
            if i in FEN:
                pos[index] = FEN[i]
                index+=1
            else:
                try:
                    for v in range(int(i)):
                        pos[index] = 0
                        index+=1
                except ValueError:
                    if i == ' ':
                        info = True
    return pos
def pos_to_fen(pos, turn, castle, en_passant, movecount, halfmovecount):
        blank = 0
        square = 0
        fen = ''

        for i in pos:
            if i == 0:
                if blank == 8:
                    fen += '8'
                    blank = 0
                else:
                    blank += 1

            elif blank>0:
                fen += str(blank)
                blank=0
                fen += POS[i]

            elif blank==0:
                fen += POS[i]
        
            if square%8 == 7 and square!=63:
                if blank>0:
                    fen+=str(blank)
                    blank = 0
                fen += '/'
            square+=1

        #adding self information (standard fen notation)
        fen += ' ' + turn + ' ' + ''.join(castle).strip(' ') + ' ' + ALGEBRAIC[en_passant] + ' ' + str(movecount) + ' ' + str(halfmovecount)
        print(fen)
        return fen 
def getmovesource(move):
    return move & 0x3f
def getmovetarget(move):
    #(legal_moves.moves[i] & 0xfc0) >> 6
    return (move & 0xfc0) >> 6
def getmovepiece(move):
    return (move & 0xf000) >> 12
def getmovepromoted(move):
    return ((move & 0xf0000) >> 16)
def getmovecapture(move):
    return (move & 0x100000)
def getmovedouble(move):
    return (move & 0x200000)
def getmoveenpassant(move):
    return (move & 0x400000)
def getmovecastling(move):
    return (move & 0x800000)
class Board():
    def __init__(self, root):
        self.root = root
        self.fen = DEFAULT_BOARD
        
        #images
        self.dot = tk.PhotoImage(file=f'{FOLDER}/assets/legal.png')

        self.boardframe = tk.Canvas(self.root)
        self.load_ui()
        self.boardframe.place(relx=0.01, rely=0.015)

        self.images = {}
        self.dots = {}
        self.squares = []
        self.base_squares = []
        self.legals = {}
        self.pieces = {}
        self.piece = 0 #piece selected
        self.lastclicked = None

        #game information
        self.board = [0]*64
        self.turn = 'w'
        self.color = Piece.white
        self.enpassant = -1
        self.moves = {}
        self.castle = ['K', 'Q', 'k', 'q']
        self.halfmoves = 1
        self.fullmoves = 0
        self.clone = None

        s = 0
        for i in self.board:
            x = s % 8
            y = s // 8
            colour = 'steelblue1' if (x+y)%2 == 1 else 'grey95'

            base_square = tk.Canvas(self.boardframe, bg=colour, width=75, height=75)
            base_square.grid(column=x, row=y)
            square = tk.Canvas(self.boardframe, bg=colour, width=75, height=75)
            square.grid(column=x, row=y)
            square.bind('<ButtonPress-1>', self.on_click)
            square.bind('<B1-Motion>', self.on_drag)
            square.bind('<ButtonRelease-1>', self.on_drop)

            square.__square__  = s
            square.__piece__ = i

            square.tkraise(0)
            self.squares.append(square)
            s += 1
        self.load_fen(self.fen)
    
    def load_ui(self):
        fenbox = ttk.LabelFrame(self.root, width=250, height=200, text="FEN")
        fenbox.place(relx=0.7, rely=0.04)
        fenbox.pack_propagate(False)
        
        enterfen = ttk.Entry(fenbox, text="Enter FEN")
        loadcopy = ttk.Frame(fenbox)
        loadfen = ttk.Button(loadcopy, text="Load FEN", command=lambda: self.load_fen(enterfen.get()))
        copyfen = ttk.Button(loadcopy, text="Copy FEN", command=lambda: self.root.clipboard_append(self.fen))
        enterfen.pack(fill='x', padx=5, pady=(2,5))
        loadcopy.pack(fill='x', padx=5)
        loadfen.pack(side='left', fill='x', padx=5, expand=True)
        copyfen.pack(fill='x', expand=True, padx=5)
        flipboard = ttk.Button(fenbox, text="Flip Board", command=lambda: self.flip_board())
        flipboard.pack(fill='x', padx=15, pady=(20,2))
        resetboard = ttk.Button(fenbox, text="Reset Board", command=self.reset_board)
        resetboard.pack(fill='x', padx=15, pady=(2,2))

        movesbox = ttk.LabelFrame(self.root, width=250, height=420, text="Moves")
        movesbox.pack_propagate(False)
        movesbox.place(relx=0.7, rely=0.35)
        self.movelist = ttk.Treeview(movesbox, column=('w', 'b'), show="headings") 
        scrollbar = ttk.Scrollbar(orient="vertical",command=self.movelist.yview)
        self.movelist.configure(yscrollcommand=scrollbar.set)
        self.movelist.column('w', width=90)
        self.movelist.column('b', width=90)
        self.movelist.heading('w', text="White")
        self.movelist.heading('b', text="Black")
        self.movelist.pack(fill='both', expand=True, padx=5, pady=5)
        left = ttk.Button(movesbox, text="<", command=self.prev_move)
        right = ttk.Button(movesbox, text=">", command=self.next_move)
        left.pack(side='left', fill='x', expand=True, padx=10, pady=10)
        right.pack(side='right', fill='x', expand=True, padx=10, pady=10)

        resign = ttk.Label(movesbox, image=tk.PhotoImage(f'{FOLDER}/assets/resign.png'))
        resign.place(relx=0.5, rely=0.2)
        resign.bind('<Button-1>')

        self.fenlabel = ttk.Label(self.root, text="")
        self.fenlabel.place(relx=0.02, rely=0.95)

    def load_fen(self, fen, fromc=False):
        print(fen)
        if fromc == False:
            c_functions.parse_fen(fen.encode('UTF-8'))
        else:
            fen = fen.decode('UTF-8')
            print(fen)
        self.fen = fen

        self.fenlabel.configure(text=fen)
        self.board = fen_to_board(fen)
        s = 0

        for i in self.board:
            self.squares[s].delete('all')
            if i != Piece.blank:
                if not(i in self.images):
                    self.images[i] = tk.PhotoImage(file=f'{FOLDER}/assets/{i}.png')
                image = self.squares[s].create_image(39,39, image=self.images[i])
                self.squares[s].__piece__ = i
                self.pieces[i] = image
            s += 1

    def reset_board(self):
        self.board = [0]*64
        self.turn = 'w'
        self.color = Piece.white
        self.enpassant = -1
        self.moves = {}
        self.castle = ['K', 'Q', 'k', 'q']
        self.halfmoves = 0
        self.fullmoves = 1
        self.load_fen(DEFAULT_BOARD)

    def random_fen(self, fen):
        pass
    def next_move(self):
        pass
    def prev_move(self):
        pass
    def this_move(self, move):
        pass
    def add_move(self):
        pass

    def clone_widget(self, widget):
        parent = widget.nametowidget(widget.winfo_parent())
        clone = widget.__class__(parent)

        for key in widget.configure():
            clone.configure({key: widget.cget(key)})
        try:
            clone.create_image(39,39,image=self.images[widget.__piece__])
        except KeyError as e:
            print(clone)
        return clone

    def clear_dots(self):
        for i in self.dots:
            self.squares[i].delete(self.dots[i])
        self.dots = {}

    def on_click(self, event):
        widget = event.widget
        widget._drag_start_x = event.x
        widget._drag_start_y = event.y
        
        self.clear_dots()
        try:
            square = str(event.widget.__square__)
        except AttributeError as e:
            return
        square = int(square)

        if ALGEBRAIC[square] in self.legals:
            self.make_move(self.lastclicked, square)
        
        self.legals = {}
        abs_piece = self.board[square]
        colour = Piece.white if Piece.is_colour(abs_piece) else Piece.black
        self.piece = abs_piece-colour

        legal_moves = c_functions.get_moves(square)
        self.colour = colour
        self.lastclicked = square

        print(ALGEBRAIC[square])
        for i in range(legal_moves.count):
            target = (legal_moves.moves[i] & 0xfc0) >> 6
            self.legals[(ALGEBRAIC[target])] = legal_moves.moves[i]
            image = self.squares[target].create_image(40,40,image=self.dot)
            self.dots[target] = image
            print(self.legals)

        widget.grid_forget()
        self.clone = self.clone_widget(widget)
        x = widget.winfo_x() - widget._drag_start_x + event.x
        y = widget.winfo_y() - widget._drag_start_y + event.y
        self.clone.place(x=x, y=y)
    def on_drag(self, event):
        if self.clone != None:
            widget = event.widget
            x = widget.winfo_x() - widget._drag_start_x + event.x
            y = widget.winfo_y() - widget._drag_start_y + event.y
            self.clone.place(x=x, y=y)
    def on_drop(self, event):
        widget = event.widget
        x, y = widget.winfo_pointerxy()
        widget.grid(column=(widget.__square__ ) % 8, row=(widget.__square__) // 8)
        if self.clone != None:
            self.clone.grid_forget()
            self.clone.destroy()
            self.clone = None

        target = self.root.winfo_containing(x, y)

        try:
            square = widget.__square__
            target_square = target.__square__
        except AttributeError as e:
            return

        self.clear_dots()
        self.make_move(square, target_square)
    
    def make_move(self, source, target):
        if ALGEBRAIC[target] in self.legals:

            '''
            if getmovecastling(self.legals[ALGEBRAIC[target]]):
                match (ALGEBRAIC[target]):
                    case 'g1':
                        self.castle.remove('K')
                        self.castle.remove('Q')
                        self.board[LETTERS_TO_NUM['h1']] = Piece.blank
                        self.board[LETTERS_TO_NUM['f1']] = Piece.rook + (Piece.white + 1 if self.color == Piece.white else 1)
                    case 'c1': self.castle.remove('Q')
                    case 'g8': self.castle.remove('k')
                    case 'c8': self.castle.remove('q')
            '''
            self.board[target] = self.board[source] 
            self.board[source] = Piece.blank

            self.turn = 'b' if self.turn == 'w' else 'w'
            self.fullmoves += 1 if self.turn == 'w' else 0
            if self.fullmoves != 0:
                self.halfmoves += 1
            self.load_fen(c_functions.py_makemove(self.legals[ALGEBRAIC[target]]), fromc=True)

            if self.turn == 'b':
                self.movelist.insert('', self.fullmoves, values=(str(self.fullmoves) +'. e4', ''))
            else:
                self.movelist.delete(self.movelist.get_children()[-1])
                self.movelist.insert('', self.fullmoves - 1, values=(str(self.fullmoves-1) + '. e4' , "e5")) 


def main():
    root = tk.Tk()
    root.title('Chess')
    root.geometry('950x700')
    root.maxsize(950, 700)
    root.minsize(950, 700)
    root.iconbitmap(FOLDER+'/assets/icon.ico')

    sv_ttk.set_theme("dark")
    
    board = Board(root)
    root.mainloop()

main()