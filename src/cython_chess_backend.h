/* 
@author: Ranuja Pinnaduwage

This file is part of cython-chess, a Cython-optimized modification of python-chess.

Description:
This header file defines the C++ helper functions to be injected into the main Cython file

Based on python-chess: https://github.com/niklasf/python-chess  
Copyright (C) 2025 Ranuja Pinnaduwage  
Licensed under the GNU General Public License (GPL) v3 or later.  

This program is distributed in the hope that it will be useful,  
but WITHOUT ANY WARRANTY; without even the implied warranty of  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
See the GNU General Public License for more details.  

You should have received a copy of the GNU General Public License  
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CYTHON_CHESS_BACKEND
#define CYTHON_CHESS_BACKEND

#include <vector>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <string>
#include <cstring>

/*
	Set of functions to initialize masks for move generation
*/
void initialize_attack_tables();
void attack_table(const std::vector<int8_t>& deltas, std::vector<uint64_t> &mask_table, std::vector<std::unordered_map<uint64_t, uint64_t>> &attack_table);
uint64_t sliding_attacks(uint8_t square, uint64_t occupied, const std::vector<int8_t>& deltas);
void carry_rippler(uint64_t mask, std::vector<uint64_t> &subsets);
void rays(std::vector<std::vector<uint64_t>> &rays);
uint64_t edges(uint8_t square);

/*
	Set of functions used to generate moves
*/
void generate_piece_moves(std::vector<uint8_t> &startPos, std::vector<uint8_t> &endPos, uint64_t our_pieces, uint64_t pawnsMask, uint64_t knightsMask, uint64_t bishopsMask, uint64_t rooksMask, uint64_t queensMask, uint64_t kingsMask, uint64_t occupied_whiteMask, uint64_t occupied_blackMask, uint64_t occupiedMask, uint64_t from_mask, uint64_t to_mask);
void generate_pawn_moves(std::vector<uint8_t> &startPos, std::vector<uint8_t> &endPos, std::vector<uint8_t> &promotions, uint64_t opposingPieces, uint64_t occupied, bool colour, uint64_t pawnsMask, uint64_t from_mask, uint64_t to_mask);
uint64_t attackers_mask(bool colour, uint8_t square, uint64_t occupied, uint64_t queens_and_rooks, uint64_t queens_and_bishops, uint64_t kings, uint64_t knights, uint64_t pawns, uint64_t occupied_co);
uint64_t slider_blockers(uint8_t king, uint64_t queens_and_rooks, uint64_t queens_and_bishops, uint64_t occupied_co_opp, uint64_t occupied_co, uint64_t occupied);
uint64_t betweenPieces(uint8_t a, uint8_t b);
uint64_t ray(uint8_t a, uint8_t b);

/*
	Set of functions used as utilities for all above functions
*/
uint8_t count_leading_zeros(uint64_t value);
uint8_t num_pieces(uint64_t bb);
void scan_reversed(uint64_t bb, std::vector<uint8_t> &result);
std::vector<uint8_t> scan_reversedOld(uint64_t bb);
void scan_forward(uint64_t bb, std::vector<uint8_t> &result);
uint64_t attacks_mask(bool colour, uint64_t occupied, uint8_t square, uint8_t pieceType);
uint8_t square_distance(uint8_t sq1, uint8_t sq2);

#endif // CYTHON_CHESS_BACKEND
