# -*- coding: utf-8 -*-
"""
Created on Fri Mar 28 12:59:11 2025

@author: Kumodth
"""
from timeit import default_timer as timer
import Cython_Chess as cython_chess
import chess
# board = cython_chess.board()
import sys
print(sys.path)  # This shows where Python is searching for packages

cython_chess.initialize()
board = chess.Board()
t0= timer()
# #for move in Cython_Chess.pseudo_legal_moves(board):
for i in range (100000):
    for move in cython_chess.generate_legal_moves(board,chess.BB_ALL,chess.BB_ALL):
        # print(move)
        # if (board.is_capture(move)):
        #     pass
        # board.push(move)
        # print(move, Cython_Chess.is_checkmate(board))
        # print(board)
        # board.pop()
        pass
t1 = timer()
print("Time elapsed: ", t1 - t0)

t0= timer()
for i in range (100000):
    for move in cython_chess.generate_legal_moves(board,chess.BB_ALL,chess.BB_ALL):
        # if (cython_chess.is_capture(board, move)):
        #     pass
        # print(move)
        # if (board.is_checkmate()):
        #     pass
        # board.push(move)
        # print(move, board.is_checkmate())     
        # print(board)
        # board.pop()
        pass
t1 = timer()
print("Time elapsed: ", t1 - t0)