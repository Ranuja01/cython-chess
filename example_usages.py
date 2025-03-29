# -*- coding: utf-8 -*-
"""
Created on Fri Mar 28 12:59:11 2025

@author: Kumodth
"""
from timeit import default_timer as timer
import cython_chess
import chess

board = chess.Board()

## SPEED COMPARISONS

# Legal move generation
t0= timer()
for i in range (100000):
    for move in board.generate_legal_moves():
        pass
t1 = timer()
print("Time elapsed: ", t1 - t0)

t0= timer()
for i in range (100000):
    for move in cython_chess.generate_legal_moves(board,chess.BB_ALL,chess.BB_ALL):
        pass
t1 = timer()
print("Time elapsed: ", t1 - t0)

# Legal captures generation
t0= timer()
for i in range (100000):
    for move in board.generate_legal_captures():
        pass
t1 = timer()
print("Time elapsed: ", t1 - t0)

t0= timer()
for i in range (100000):
    for move in cython_chess.generate_legal_captures(board,chess.BB_ALL,chess.BB_ALL):
        pass
t1 = timer()
print("Time elapsed: ", t1 - t0)