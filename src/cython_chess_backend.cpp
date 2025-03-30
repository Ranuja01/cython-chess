/* 
@author: Ranuja Pinnaduwage

This file is part of cython-chess, a Cython-optimized modification of python-chess.

Description:
This file implements C++ helper functions to be injected into the main Cython file

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

#include "cython_chess_backend.h"
#include <array>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <omp.h>
#include <numeric>
#include <execution>
#include <random>
#include <deque>
#include <string>
#include <cstring>
#include <immintrin.h> // For MSVC intrinsics

constexpr int NUM_SQUARES = 64;

// Define masks for move generation
std::array<uint64_t, NUM_SQUARES> BB_KNIGHT_ATTACKS;
std::array<uint64_t, NUM_SQUARES> BB_KING_ATTACKS;
std::array<std::array<uint64_t, NUM_SQUARES>, 2> BB_PAWN_ATTACKS;
std::vector<uint64_t> BB_DIAG_MASKS;
std::vector<std::unordered_map<uint64_t, uint64_t>> BB_DIAG_ATTACKS;
std::vector<uint64_t> BB_FILE_MASKS;
std::vector<std::unordered_map<uint64_t, uint64_t>> BB_FILE_ATTACKS;
std::vector<uint64_t> BB_RANK_MASKS;
std::vector<std::unordered_map<uint64_t, uint64_t>> BB_RANK_ATTACKS;
std::vector<std::vector<uint64_t>> BB_RAYS;

// Define zobrist table, cache and insertion order for efficient hashing
uint64_t zobristTable[12][64];
std::unordered_map<uint64_t, int> moveCache;
std::deque<uint64_t> insertionOrder;

// Define caches for player move generation orders
std::unordered_map<uint64_t, std::string> OpponentMoveGenCache;
std::deque<uint64_t> OpponentMoveGenInsertionOrder;

std::unordered_map<uint64_t, std::string> curPlayerMoveGenCache;
std::deque<uint64_t> curPlayerMoveGenInsertionOrder;

// Define the file bitboards
constexpr uint64_t BB_FILE_A = 0x0101010101010101ULL << 0;
constexpr uint64_t BB_FILE_B = 0x0101010101010101ULL << 1;
constexpr uint64_t BB_FILE_C = 0x0101010101010101ULL << 2;
constexpr uint64_t BB_FILE_D = 0x0101010101010101ULL << 3;
constexpr uint64_t BB_FILE_E = 0x0101010101010101ULL << 4;
constexpr uint64_t BB_FILE_F = 0x0101010101010101ULL << 5;
constexpr uint64_t BB_FILE_G = 0x0101010101010101ULL << 6;
constexpr uint64_t BB_FILE_H = 0x0101010101010101ULL << 7;

// Array of file uint64_ts
constexpr std::array<uint64_t, 8> BB_FILES = {
    BB_FILE_A, BB_FILE_B, BB_FILE_C, BB_FILE_D, BB_FILE_E, BB_FILE_F, BB_FILE_G, BB_FILE_H
};

// Define the rank uint64_ts
constexpr uint64_t BB_RANK_1 = 0xffULL << (8 * 0);
constexpr uint64_t BB_RANK_2 = 0xffULL << (8 * 1);
constexpr uint64_t BB_RANK_3 = 0xffULL << (8 * 2);
constexpr uint64_t BB_RANK_4 = 0xffULL << (8 * 3);
constexpr uint64_t BB_RANK_5 = 0xffULL << (8 * 4);
constexpr uint64_t BB_RANK_6 = 0xffULL << (8 * 5);
constexpr uint64_t BB_RANK_7 = 0xffULL << (8 * 6);
constexpr uint64_t BB_RANK_8 = 0xffULL << (8 * 7);

// Array of rank bitboards
constexpr std::array<uint64_t, 8> BB_RANKS = {
    BB_RANK_1, BB_RANK_2, BB_RANK_3, BB_RANK_4, BB_RANK_5, BB_RANK_6, BB_RANK_7, BB_RANK_8
};

// Define global masks for piece placement
uint64_t pawns, knights, bishops, rooks, queens, kings, occupied_white, occupied_black, occupied;
/*
	Set of functions to initialize masks for move generation
*/

uint8_t count_leading_zeros_(uint64_t value) {
    #if defined(_MSC_VER)
        return _lzcnt_u64(value); // MSVC intrinsic
    #elif defined(__GNUC__) || defined(__clang__)
        return __builtin_clzll(value); // GCC/Clang built-in
    #else
        #error "Unsupported compiler"
    #endif
}

void initialize_attack_tables() {
	
	/*
		Function to initialize attack tables for move generation
	*/
	
	// Define the position deltas for knights, kings and pawn moves
    std::vector<int8_t> knight_deltas = {17, 15, 10, 6, -17, -15, -10, -6};
    std::vector<int8_t> king_deltas = {9, 8, 7, 1, -9, -8, -7, -1};
    std::vector<int8_t> white_pawn_deltas = {-7, -9};
    std::vector<int8_t> black_pawn_deltas = {7, 9};
	
	// Fill up the tables for all possible knight, king and pawn moves
    for (int sq = 0; sq < NUM_SQUARES; ++sq) {
        BB_KNIGHT_ATTACKS[sq] = sliding_attacks(sq, ~0ULL, knight_deltas);
        BB_KING_ATTACKS[sq] = sliding_attacks(sq, ~0ULL, king_deltas);
		BB_PAWN_ATTACKS[0][sq] = sliding_attacks(sq, ~0ULL, white_pawn_deltas);
        BB_PAWN_ATTACKS[1][sq] = sliding_attacks(sq, ~0ULL, black_pawn_deltas);
    }
	
	// Call the function to fill up the tables for all possible queen and rook moves
	attack_table({-9, -7, 7, 9},BB_DIAG_MASKS,BB_DIAG_ATTACKS);
	attack_table({-8, 8},BB_FILE_MASKS,BB_FILE_ATTACKS);
	attack_table({-1, 1},BB_RANK_MASKS,BB_RANK_ATTACKS);
	
	rays(BB_RAYS);
}

void attack_table(const std::vector<int8_t>& deltas, std::vector<uint64_t> &mask_table, std::vector<std::unordered_map<uint64_t, uint64_t>> &attack_table) {
    
	/*
		Function to initialize attack mask tables for diagonal, file and rank attacks
		
		Parameters:
		- deltas: A vector of position delta values, passed by reference
		- mask_table: An empty vector to hold the sliding attacks masks, passed by reference
		- attack_table: An empty vector to hold the attack subsets of the mask, passed by reference
	*/
	
	// Loop through all squares 
    for (int square = 0; square < 64; ++square) {
        std::unordered_map<uint64_t, uint64_t> attacks;
		
		// Acquire sliding attacks mask for the given deltas
        uint64_t mask = sliding_attacks(square, 0ULL, deltas) & ~edges(square);
        
		// Acquire subsets of attacks mask and loop through them to form the attack table
		std::vector<uint64_t> subsets;
		carry_rippler(mask,subsets);
        for (uint64_t subset : subsets) {
            attacks[subset] = sliding_attacks(square, subset, deltas);
        }

		// Push the current mask and attack tables to the full set
        mask_table.push_back(mask);
        attack_table.push_back(attacks);
    }
}

uint64_t sliding_attacks(uint8_t square, uint64_t occupied, const std::vector<int8_t>& deltas) {
	
	/*
		Function to calculate sliding attacks
		
		Parameters:
		- square: The starting square
		- uint64_t: The mask of occupied pieces
		- deltas: A vector of position delta values, passed by reference
		
		Returns:
		A bitboard mask representing the sliding attacks possible from the given square with the given deltas
	*/
	
    uint64_t attacks = 0ULL;

	// Loop through the deltas
    for (int8_t delta : deltas) {
        uint8_t sq = square;

		// Keep applying the delta
        while (true) {
			
			// Check if the current square either wraps around or goes outside the board to break the loop
            sq += delta;
            if (!(0 <= sq && sq < 64) || square_distance(sq, sq - delta) > 2) {
                break;
            }

			// Add the square to the attacks mask
            attacks |= (1ULL << sq);

			// If the square is occupied, the attack stops there
            if (occupied & (1ULL << sq)) {
                break;
            }
        }
    }
    return attacks;
}

void carry_rippler(uint64_t mask, std::vector<uint64_t> &subsets) {
    
	/*
		Function to generate subsets of a given mask
		
		Parameters:
		- mask: The mask to create subsets of
		- subsets: An empty vector to hold the ssubsets, passed by reference		
	*/
	
	// Generates all subsets of the bitmask iteratively    
	uint64_t subset = 0ULL;
    do {
		// This operation flips bits in subset and ensures only bits set in mask are retained.
        subsets.push_back(subset);
        subset = (subset - mask) & mask;
    } while (subset);
    
}

void rays(std::vector<std::vector<uint64_t>> &rays) {
    
	/*
		Function to attack rays
		
		Parameters:
		- rays: An empty vector to hold the vectors of squares representing each ray, passed by reference
	*/
	
	// Loop through all squares to represent starting points
    for (size_t a = 0; a < 64; ++a) {
        std::vector<uint64_t> rays_row;
        uint64_t bb_a = 1ULL << a;
		
		// Loop through all squares to represent ending points
        for (size_t b = 0; b < 64; ++b) {
            uint64_t bb_b = 1ULL << b;
			
			// Get all diagonal, rank and file attacks for the given points
            if (BB_DIAG_ATTACKS[a][0] & bb_b) {
                rays_row.push_back((BB_DIAG_ATTACKS[a][0] & BB_DIAG_ATTACKS[b][0]) | bb_a | bb_b);
            } else if (BB_RANK_ATTACKS[a][0] & bb_b) {
                rays_row.push_back(BB_RANK_ATTACKS[a][0] | bb_a);
            } else if (BB_FILE_ATTACKS[a][0] & bb_b) {
                rays_row.push_back(BB_FILE_ATTACKS[a][0] | bb_a);
            } else {
                rays_row.push_back(0ULL);
            }
        }
		
		// Push each ray to the vector
        rays.push_back(rays_row);
    }    
}

uint64_t edges(uint8_t square) {
	
	/*
		Function to get a bitmask of the edges
		
		Parameters:
		- rays: An empty vector to hold the vectors of squares representing each ray, passed by reference
	*/
	
    uint64_t rank_mask = (0xFFULL | 0xFF00000000000000ULL) & ~(0xFFULL << (8 * (square / 8)));
    uint64_t file_mask = (0x0101010101010101ULL | 0x8080808080808080ULL) & ~(0x0101010101010101ULL << square % 8);
    return rank_mask | file_mask;
}

uint8_t piece_type_at(uint8_t square){
    
	/*
		Function to get the piece type
		
		Parameters:
		- square: The square to be analyzed		
		
		Returns:
		A unsigned char representing the piece type
	*/
    
	/* 
		In this section, see if the individual piece type masks bitwise Anded with the square exists
		If so this suggests that the given square has the piece type of that mask
	*/
	uint64_t mask = (1ULL << square);

	if (pawns & mask) {
		return 1;
	} else if (knights & mask){
		return 2;
	} else if (bishops & mask){
		return 3;
	} else if (rooks & mask){
		return 4;
	} else if (queens & mask){
		return 5;
	} else if (kings & mask){
		return 6;
	} else{
		return 0;
	}
}

/*
	Set of functions used to generate moves
*/
void generate_piece_moves(std::vector<uint8_t> &startPos, std::vector<uint8_t> &endPos, uint64_t our_pieces, uint64_t pawnsMask, uint64_t knightsMask, uint64_t bishopsMask, uint64_t rooksMask, uint64_t queensMask, uint64_t kingsMask, uint64_t occupied_whiteMask, uint64_t occupied_blackMask, uint64_t occupiedMask, uint64_t from_mask, uint64_t to_mask){
    
	/*
		Function to generate moves for non-pawn pieces
		
		Parameters:
		- startPos: An empty vector to hold the starting positions, passed by reference 
		- endPos: An empty vector to hold the ending positions, passed by reference 
		- our_pieces: The mask containing only the pieces of the current side
		- pawnsMask: The mask containing only pawns
		- knightsMask: The mask containing only knights
		- bishopsMask: The mask containing only bishops
		- rooksMask: The mask containing only rooks
		- queensMask: The mask containing only queens
		- kingsMask: The mask containing only kings
		- prevKingsMask: The mask containing the position of the kings at the previous move in-game
		- occupied_whiteMask: The mask containing only white pieces
		- occupied_blackMask: The mask containing only black pieces
		- occupiedMask: The mask containing all pieces
		- from_mask: The mask of the possible starting positions for move generation
		- to_mask: The mask of the possible ending positions for move generation		
	*/
	
	// Set the masks for each piece and for all white and black pieces globally
	pawns = pawnsMask;
	knights = knightsMask;
	bishops = bishopsMask;
	rooks = rooksMask;
	queens = queensMask;
	kings = kingsMask;
	occupied_white = occupied_whiteMask;
	occupied_black = occupied_blackMask;
	occupied = occupiedMask;
	
	// Define mask of non pawn pieces
	uint64_t non_pawns = (our_pieces & ~pawns) & from_mask;
		
	// Loop through the non pawn pieces
	uint8_t r = 0;
	uint64_t bb = non_pawns;
	while (bb) {
		r = 64 - count_leading_zeros_(bb) - 1;
		
		// Define the moves as a bitwise and between the squares attacked from the starting square and the starting mask
		uint64_t moves = (attacks_mask(bool((1ULL<<r) & occupied_white),occupied,r,piece_type_at (r)) & ~our_pieces) & to_mask;		
		
		// Loop through the possible destinations
		uint8_t r_inner = 0;
		uint64_t bb_inner = moves;
		while (bb_inner) {
			r_inner = 64 - count_leading_zeros_(bb_inner) - 1;
			
			// Push the starting and ending positions to their respective vectors
			startPos.push_back(r);
			endPos.push_back(r_inner);  
			bb_inner ^= (1ULL << r_inner);
		}
		
		bb ^= (1ULL << r);
	}
}

void generate_pawn_moves(std::vector<uint8_t> &startPos, std::vector<uint8_t> &endPos, std::vector<uint8_t> &promotions, uint64_t opposingPieces, uint64_t occupied, bool colour, uint64_t pawnsMask, uint64_t from_mask, uint64_t to_mask){    			
	
	/*
		Function to generate moves for pawn pieces
		
		Parameters:
		- startPos: An empty vector to hold the starting positions, passed by reference 
		- endPos: An empty vector to hold the ending positions, passed by reference 
		- promotions: An empty vector to hold the promotion status, passed by reference
		- opposingPieces: The mask containing only the pieces of the opposing side
		- occupied: The mask containing all pieces
		- colour: the colour of the current side
		- pawnsMask: The mask containing only pawns
		- from_mask: The mask of the possible starting positions for move generation
		- to_mask: The mask of the possible ending positions for move generation		
	*/
		
	/*
		This section of code is used for pawn captures
	*/		
	// Loop through the pawns
	uint8_t r = 0;
	uint64_t bb = pawnsMask;
	while (bb) {
		r = 64 - count_leading_zeros_(bb) - 1;
		
		// Acquire the destinations that follow pawn attacks and opposing pieces
		uint64_t moves = BB_PAWN_ATTACKS[colour][r] & opposingPieces & to_mask;
		
		// Loop through the destinations 
		uint8_t r_inner = 0;
		uint64_t bb_inner = moves;
		while (bb_inner) {
			r_inner = 64 - count_leading_zeros_(bb_inner) - 1;
			
			// Check if the rank suggests the move is a promotion
			uint8_t rank = r_inner / 8;			
			if (rank == 7 || rank == 0){
				
				// Loop through all possible promotions
				for (int k = 5; k > 1; k--){
					startPos.push_back(r);
					endPos.push_back(r_inner);
					promotions.push_back(k);
				}
			
			// Else the move is not a promotion
			} else{
				startPos.push_back(r);
				endPos.push_back(r_inner);
				promotions.push_back(1);
			}  
			bb_inner ^= (1ULL << r_inner);
		}
		
		bb ^= (1ULL << r);
	}
	
	/*
		In this section, define single and double pawn pushes
	*/
	uint64_t single_moves, double_moves;
	if (colour){
        single_moves = pawnsMask << 8 & ~occupied;
        double_moves = single_moves << 8 & ~occupied & (BB_RANK_3 | BB_RANK_4);
    }else{
        single_moves = pawnsMask >> 8 & ~occupied;
        double_moves = single_moves >> 8 & ~occupied & (BB_RANK_6 | BB_RANK_5);
	}
    
	single_moves &= to_mask;
    double_moves &= to_mask;
	
	/*
		This section of code is used for single pawn pushes
	*/		
	// Loop through the pawns
	r = 0;
	bb = single_moves;
	while (bb) {
		r = 64 - count_leading_zeros_(bb) - 1;
		
		// Set the destination square as either one square up or down the board depending on the colour
		uint8_t from_square = r;
		if (colour){
			from_square -= 8;
		} else{
			from_square += 8;
		}
		
		// Check if the rank suggests the move is a promotion
		uint8_t rank = r / 8;
		if (rank == 7 || rank == 0){	

			// Loop through all possible promotions		
			for (int j = 5; j > 1; j--){
				startPos.push_back(from_square);
				endPos.push_back(r);
				promotions.push_back(j);
			}
		// Else the move is not a promotion
		} else{
			startPos.push_back(from_square);
			endPos.push_back(r);
			promotions.push_back(1);
		}
		bb ^= (1ULL << r);
	}
	
	/*
		This section of code is used for double pawn pushes
	*/		
	// Loop through the pawns
	r = 0;
	bb = double_moves;
	while (bb) {
		r = 64 - count_leading_zeros_(bb) - 1;
		
		// Set the destination square as either two squares up or down the board depending on the colour
		uint8_t from_square = r;
		if (colour){
			from_square -= 16;
		} else{
			from_square += 16;
		}
		
		// Set the start and destination
		startPos.push_back(from_square);
		endPos.push_back(r);
		promotions.push_back(1);
		bb ^= (1ULL << r);
	}
	
}

uint64_t attackers_mask(bool colour, uint8_t square, uint64_t occupied, uint64_t queens_and_rooks, uint64_t queens_and_bishops, uint64_t kings, uint64_t knights, uint64_t pawns, uint64_t occupied_co){
    
	/*
		Function to generate a mask containing attacks from the given square
		
		Parameters:
		- colour: The colour of the opposing side
		- square: The starting square
		- occupied: The mask containing all pieces
		- queens_and_rooks: The mask containing queens and rooks
		- queens_and_bishops: The mask containing queens and bishops
		- kings: The mask containing only kings
		- knights: The mask containing only knights
		- pawns: The mask containing only pawns
		- occupied_co: The mask containing the pieces of the opposing side
		
		Returns:
		A mask containing attacks from the given square
	*/
	
	// Acquire the masks of the pieces on the same rank, file and diagonal as the given square
	uint64_t rank_pieces = BB_RANK_MASKS[square] & occupied;
    uint64_t file_pieces = BB_FILE_MASKS[square] & occupied;
    uint64_t diag_pieces = BB_DIAG_MASKS[square] & occupied;

	// Acquire all attack masks for each piece type
    uint64_t attackers = (
        (BB_KING_ATTACKS[square] & kings) |
        (BB_KNIGHT_ATTACKS[square] & knights) |
        (BB_RANK_ATTACKS[square][rank_pieces] & queens_and_rooks) |
        (BB_FILE_ATTACKS[square][file_pieces] & queens_and_rooks) |
        (BB_DIAG_ATTACKS[square][diag_pieces] & queens_and_bishops) |
        (BB_PAWN_ATTACKS[!colour][square] & pawns));

	// Perform a bitwise and with the opposing pieces 
    return attackers & occupied_co;
}

uint64_t slider_blockers(uint8_t king, uint64_t queens_and_rooks, uint64_t queens_and_bishops, uint64_t occupied_co_opp, uint64_t occupied_co, uint64_t occupied){    
    
	/*
		Function to generate a mask that represents check blocking pieces
		
		Parameters:
		- king: The square of the current side's king
		- queens_and_rooks: The mask containing queens and rooks
		- queens_and_bishops: The mask containing queens and bishops
		- occupied_co_opp: The mask containing only pieces from the opposition
		- occupied_co: The mask containing only pieces from the current side
		- occupied_co: The mask containing the pieces of the opposing side
		
		Returns:
		A mask containing check blockers
	*/
	
	// Define a mask for possible sliding attacks on the king square
    uint64_t snipers = ((BB_RANK_ATTACKS[king][0] & queens_and_rooks) |
                (BB_FILE_ATTACKS[king][0] & queens_and_rooks) |
                (BB_DIAG_ATTACKS[king][0] & queens_and_bishops));

	// Define a mask containing possible squares to block the attack
    uint64_t blockers = 0;
	
	
	// Loop through the mask containing sniper attackers
	uint8_t r = 0;
	uint64_t bb = snipers & occupied_co_opp;
	while (bb) {
		r = 64 - count_leading_zeros_(bb) - 1;
		
		// Update the blockers where there is exactly one blocker per attack 
		uint64_t b = betweenPieces(king, r) & occupied;        
        if (b && (1ULL << (63 - count_leading_zeros_(b)) == b)){
            blockers |= b;
		}
		bb ^= (1ULL << r);
	}
	
	// Return the blockers where the square contains a piece of the current side
    return blockers & occupied_co;
}
	
uint64_t betweenPieces(uint8_t a, uint8_t b){
		
	/*
		Function to get a mask of the squares between a and b
		
		Parameters:
		- a: The starting square
		- b: The ending square
		
		Returns:
		A mask containing the squares between a and b
	*/
	
	// Use BB_RAYS to get a mask of all square in the same ray as a and back
	// Use (~0x0ULL << a) ^ (~0x0ULL << b) to get all square between a and b in counting order
    uint64_t bb = BB_RAYS[a][b] & ((~0x0ULL << a) ^ (~0x0ULL << b));
	
	// Remove a and b themselves
    return bb & (bb - 1);
}

uint64_t ray(uint8_t a, uint8_t b){return BB_RAYS[a][b];}

/*
	Set of functions used as utilities for all above functions
*/
void scan_reversed(uint64_t bb, std::vector<uint8_t> &result){	
		
	/*
		Function to acquire a vector of set bits in a given bitmask starting from the top of the board
		
		Parameters:
		- bb: The bitmask to be scanned
		- result: An empty vector to hold the set bits, passed by reference
	*/
	
	uint8_t r = 0;
    while (bb) {
        r = 64 - count_leading_zeros_(bb) - 1;
        result.push_back(r);
        bb ^= (1ULL << r);		
    }    
}

void scan_forward(uint64_t bb, std::vector<uint8_t> &result) {
		
	/*
		Function to acquire a vector of set bits in a given bitmask starting from the bottom of the board
		
		Parameters:
		- bb: The bitmask to be scanned
		- result: An empty vector to hold the set bits, passed by reference
	*/
	
	uint64_t r = 0;
    while (bb) {
        r = bb &-bb;
        result.push_back(64 - count_leading_zeros_(r) - 1);
        bb ^= r;
    }
}

uint8_t num_pieces(uint64_t bb) {
		
	/*
		Function to acquires the number of set bits in a bitmask
		
		Parameters:
		- bb: The bitmask to be examined
	*/
	
    return __builtin_popcountll(bb);
}

uint8_t square_distance(uint8_t sq1, uint8_t sq2) {
		
	/*
		Function to acquire the Chebyshev distance (king's distance)
		
		Parameters:
		- sq1: The starting square
		- sq2: The ending square
		
		Returns:
		The distance that a king would have to travel to travel the squares
	*/
		
    int file_distance = abs((sq1 % 8) - (sq2 % 8));
    int rank_distance = abs((sq1 / 8) - (sq2 / 8));
    return std::max(file_distance, rank_distance);
}

uint64_t attacks_mask(bool colour, uint64_t occupied, uint8_t square, uint8_t pieceType){	
	
	/*
		Function to acquire an attack mask for a given piece on a given square
		
		Parameters:
		- colour: The colour of the current side
		- occupied: The mask containing all pieces
		- square: The current square to be analyzed
		- pieceType: The piece type on the given square
		
		Returns:
		A boolean defining whether the move is a check
	*/
	
	if (pieceType == 1){		
		return BB_PAWN_ATTACKS[colour][square];
	}else if (pieceType == 2){
		return BB_KNIGHT_ATTACKS[square];
	}else if (pieceType == 6){
		
		return BB_KING_ATTACKS[square];
	}else{
		uint64_t attacks = 0;
		if (pieceType == 3 || pieceType == 5){
			attacks = BB_DIAG_ATTACKS[square][BB_DIAG_MASKS[square] & occupied];
		}
		if (pieceType == 4 || pieceType == 5){			
			attacks |= (BB_RANK_ATTACKS[square][BB_RANK_MASKS[square] & occupied] |
						BB_FILE_ATTACKS[square][BB_FILE_MASKS[square] & occupied]);
		}
		return attacks;
	}
}