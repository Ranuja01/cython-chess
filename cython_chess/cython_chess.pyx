# -*- coding: utf-8 -*-
"""
@author: Ranuja Pinnaduwage

This file is part of cython-chess, a Cython-optimized modification of python-chess.

Description:
This file implements Cythonized versions of core python-chess components, improving performance 
with Cython and C++ while maintaining the same functionality.

Based on python-chess: https://github.com/niklasf/python-chess  
Copyright (C) 2025 Ranuja Pinnaduwage  
Licensed under the GNU General Public License (GPL) v3 or later.  

This program is distributed in the hope that it will be useful,  
but WITHOUT ANY WARRANTY; without even the implied warranty of  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
See the GNU General Public License for more details.  

You should have received a copy of the GNU General Public License  
along with this program. If not, see <https://www.gnu.org/licenses/>.
"""

cimport cython
import chess
import itertools
from collections.abc import Iterator

# From the c++ standard library import vectors and strings
from libcpp.vector cimport vector
from libcpp.string cimport string

# Define C data types 
cdef extern from "stdint.h":
    ctypedef signed char int8_t
    ctypedef unsigned char uint8_t
    ctypedef unsigned long long uint64_t

# Import functions from the c++ file
cdef extern from "cython_chess_backend.h":
    
    void initialize_attack_tables()
    uint8_t num_pieces(uint64_t bb)
    void scan_reversed(uint64_t bb, vector[uint8_t] &result)
    void scan_forward(uint64_t bb, vector[uint8_t] &result)
    uint64_t attacks_mask(bint colour, uint64_t occupied, uint8_t square, uint8_t pieceType)
    uint64_t attackersMask(bint colour, uint8_t square, uint64_t occupied, uint64_t queens_and_rooks, uint64_t queens_and_bishops, uint64_t kings, uint64_t knights, uint64_t pawns, uint64_t occupied_co)
    uint64_t slider_blockers(uint8_t king, uint64_t queens_and_rooks, uint64_t queens_and_bishops, uint64_t occupied_co_opp, uint64_t occupied_co, uint64_t occupied)
    uint64_t betweenPieces(uint8_t a, uint8_t b)
    uint64_t ray(uint8_t a, uint8_t b)
    bint is_capture(uint8_t from_square, uint8_t to_square, uint64_t occupied_co, bint is_en_passant)
    bint is_check(bint colour, uint64_t occupied, uint64_t queens_and_rooks, uint64_t queens_and_bishops, uint64_t kings, uint64_t knights, uint64_t pawns, uint64_t opposingPieces)
    void generatePieceMoves(vector[uint8_t] &startPos, vector[uint8_t] &endPos, uint64_t our_pieces, uint64_t pawnsMask, uint64_t knightsMask, uint64_t bishopsMask, uint64_t rooksMask, uint64_t queensMask, uint64_t kingsMask, uint64_t occupied_whiteMask, uint64_t occupied_blackMask, uint64_t occupiedMask, uint64_t from_mask, uint64_t to_mask)
    void generatePawnMoves(vector[uint8_t] &startPos, vector[uint8_t] &endPos, vector[uint8_t] &promotions, uint64_t opposingPieces, uint64_t occupied, bint colour, uint64_t pawnsMask, uint64_t from_mask, uint64_t to_mask)


def inititalize():
    initialize_attack_tables()    

def generate_legal_moves(object board, uint64_t from_mask, uint64_t to_mask) -> Iterator[chess.Move]:
    
    """
    Function to generate legal moves through yielding

    Parameters:
    - board: The current board state
    - from_mask: The starting position mask
    - to_mask: The ending position mask

    Yields:
    - Legal chess.Moves
    """
    
    # Define variables for the king piece and masks for king blocking pieces and checkers
    cdef uint8_t king
    cdef uint64_t blockers
    cdef uint64_t checkers
    cdef object move
    
    # Check if the game is over
    if board.is_variant_end():
        return

    # Acquire the mask for the current sides king square
    cdef uint64_t king_mask = board.kings & board.occupied_co[board.turn]
    if king_mask:
        
        
        king = king_mask.bit_length() - 1
        
        # Call the c++ function to acquire the blockers and checkers masks
        blockers = slider_blockers(king, board.queens | board.rooks, board.queens | board.bishops, board.occupied_co[not board.turn], board.occupied_co[board.turn], board.occupied)                
        checkers = attackersMask(not board.turn, king, board.occupied, board.queens | board.rooks, board.queens | board.bishops, board.kings, board.knights, board.pawns, board.occupied_co[not board.turn])
        
        # If there are pieces checking the king, generate evasions that do not keep the king in check
        if checkers:
            for move in _generate_evasions(board, king, checkers, from_mask, to_mask):
                if board._is_safe(king, blockers, move):
                    yield move
        else: # If there is not, generate pseudo legal moves that do not put the own king in check
            for move in generate_pseudo_legal_moves(board, from_mask, to_mask):            
                if board._is_safe(king, blockers, move):
                    yield move
    else:
        yield from generate_pseudo_legal_moves(board, from_mask, to_mask)

def generate_legal_ep(object board, uint64_t from_mask, uint64_t to_mask) -> Iterator[chess.Move]:
    
    """
    Function to generate legal en passent moves through yielding

    Parameters:
    - board: The current board state
    - from_mask: The starting position mask
    - to_mask: The ending position mask

    Yields:
    - Legal en passent chess.Moves
    """
    
    cdef object move
    if board.is_variant_end():
        return
    
    for move in board.generate_pseudo_legal_ep(from_mask, to_mask):
        if not board.is_into_check(move):
            yield move

def generate_legal_captures(object board, uint64_t from_mask, uint64_t to_mask) -> Iterator[chess.Move]:
    
    """
    Function to generate legal captures through yielding

    Parameters:
    - board: The current board state
    - from_mask: The starting position mask
    - to_mask: The ending position mask

    Yields:
    - Legal capture chess.Moves
    """
    
    # Use itertools to yield moves from the legal moves function and en passent function        
    return itertools.chain(
        generate_legal_moves(board,from_mask, to_mask & board.occupied_co[not board.turn]),
        generate_legal_ep(board,from_mask, to_mask))        

def _generate_evasions(object board, uint8_t king, uint64_t checkers, uint64_t from_mask, uint64_t to_mask) -> Iterator[chess.Move]:
    
    """
    Function to generate evasions through yielding

    Parameters:
    - board: The current board state
    - king: The square where the current side's king is located
    - checkers: The mask for pieces checking the king 
    - from_mask: The starting position mask
    - to_mask: The ending position mask

    Yields:
    - Legal evasion chess.Moves
    """
    
    # Define mask for sliding pieces which are also checkers
    cdef uint64_t sliders = checkers & (board.bishops | board.rooks | board.queens)
    
    # Define mask to hold ray attacks towards the king
    cdef uint64_t attacked = 0
    
    # Define variable to hold squares to stop check through either blocking or capture
    cdef uint64_t target
    cdef uint8_t last_double
    cdef uint8_t checker
    cdef vector[uint8_t] attackedVec
    cdef vector[uint8_t] moveVec
    
    # Call c++ function to scan the sliders bitmask
    scan_reversed(sliders,attackedVec)
    cdef uint8_t size = attackedVec.size()
    
    # Acquire ray attacks 
    for i in range(size):
        attacked |= ray(king, attackedVec[i]) & ~chess.BB_SQUARES[int(attackedVec[i])]

    # Return moves the king can make to evade check
    if (1<<king) & from_mask:
        
        moveVec.clear()
        scan_reversed(chess.BB_KING_ATTACKS[king] & ~board.occupied_co[board.turn] & ~attacked & to_mask, moveVec)
        size = moveVec.size()
        for i in range(size):
            yield chess.Move(king, moveVec[i])    

    checker = chess.msb(checkers)
    
    if chess.BB_SQUARES[checker] == checkers:
        # Capture or block a single checker.
        target = betweenPieces(king, checker) | checkers

        yield from generate_pseudo_legal_moves(board, ~board.kings & from_mask, target & to_mask)
        
        # Capture the checking pawn en passant (but avoid yielding
        # duplicate moves).
        if board.ep_square and not chess.BB_SQUARES[board.ep_square] & target:
            last_double = board.ep_square + (-8 if board.turn == chess.WHITE else 8)
            if last_double == checker:
                yield from board.generate_pseudo_legal_ep(from_mask, to_mask)

def generate_pseudo_legal_moves(object board, uint64_t from_mask, uint64_t to_mask) -> Iterator[chess.Move]:
    
    """
    Function to generate pseudo legal moves through yielding

    Parameters:
    - board: The current board state
    - from_mask: The starting position mask
    - to_mask: The ending position mask

    Yields:
    - Legal evasion chess.Moves
    """
    
    # Define masks for our pieces and all pieces
    cdef uint64_t our_pieces = board.occupied_co[board.turn]
    cdef uint64_t all_pieces = board.occupied
    
    # Define vectors to hold different pseudo legal moves
    cdef vector[uint8_t] pieceVec
    cdef vector[uint8_t] pieceMoveVec
    cdef vector[uint8_t] pawnVec
    cdef vector[uint8_t] pawnMoveVec
    cdef vector[uint8_t] promotionVec

    cdef uint8_t outterSize
    cdef uint8_t innerSize
   
    # Call the c++ function to generate piece moves.
    generatePieceMoves(pieceVec, pieceMoveVec, our_pieces, board.pawns, board.knights, board.bishops, board.rooks, board.queens, board.kings, board.occupied_co[True], board.occupied_co[False], board.occupied, from_mask, to_mask)
    outterSize = pieceVec.size()  
    
    for i in range(outterSize):
        yield chess.Move(pieceVec[i], pieceMoveVec[i])
    
    # Call the c++ function to generate castling moves.
    if from_mask & board.kings:
        yield from board.generate_castling_moves(from_mask, to_mask)

    # The remaining moves are all pawn moves.
    cdef uint64_t pawns = board.pawns & board.occupied_co[board.turn] & from_mask
    if not pawns:
        return

    generatePawnMoves(pawnVec, pawnMoveVec, promotionVec, board.occupied_co[not board.turn], board.occupied, board.turn, pawns, from_mask, to_mask)
    outterSize = pawnVec.size()  
    for i in range(outterSize):
        if (promotionVec[i] == 1):
            yield chess.Move(pawnVec[i], pawnMoveVec[i])
        else:
            yield chess.Move(pawnVec[i], pawnMoveVec[i], promotionVec[i])

    # Call the c++ function to generate en passant captures.
    if board.ep_square:
        yield from board.generate_pseudo_legal_ep(from_mask, to_mask)

def gives_check(object board,object move):
    board.push(move)
    try:
        return is_check(board.turn,board.occupied, board.queens | board.rooks, board.queens | board.bishops, board.kings, board.knights, board.pawns, board.occupied_co[not board.turn])    
    finally:
        board.pop()
