# -*- coding: utf-8 -*-
"""
Created on Thu Mar 27 18:01:06 2025

@author: Kumodth
"""

import Cython_Chess

print(dir(Cython_Chess))

from timeit import default_timer as timer
import chess

board = chess.Board()
Cython_Chess.inititalize()
t0= timer()
# #for move in Cython_Chess.pseudo_legal_moves(board):
for i in range (100000):
    for move in board.generate_legal_moves():
        # print(move)
        # if (Cython_Chess.is_checkmate(board)):
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
    for move in Cython_Chess.generate_legal_moves(board,chess.BB_ALL,chess.BB_ALL):
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