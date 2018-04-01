/* Copyright 2018 Romain "Artefact2" Dal Maso <artefact2@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#ifndef ORMULOGUN_H
#define ORMULOGUN_H

#include <stdio.h>
#include <stdbool.h>
#include <gumble.h>

#define ORM_MOVES_EQUAL(m1, m2) ((m1).start == (m2).start && (m1).end == (m2).end && (m1).promote == (m2).promote)

/* ----- uci.c ----- */

typedef struct {
	FILE* r;
	FILE* w;
} uci_engine_context_t;

typedef struct {
	enum { SCORE_CP, SCORE_MATE } type;
	int score;
	char bestlan[SAFE_ALG_LENGTH];
} uci_eval_t;

int uci_create(const char*, uci_engine_context_t*);
void uci_quit(const uci_engine_context_t*);
void uci_init(const uci_engine_context_t*, const char* const*);
unsigned char uci_eval(const uci_engine_context_t*, const char*, cch_board_t*, uci_eval_t*, unsigned char);



/* ----- puzzle.c ----- */

typedef struct {
	int min_eval_cutoff;
	int max_eval_cutoff;
	int puzzle_threshold_absolute;
	int puzzle_threshold_absolute_diff;
	float variation_cutoff_relative;
	unsigned int min_ply;
	unsigned char max_variations;
	unsigned char max_depth;
} puzzlegen_settings_t;

typedef struct puzzle_step_s {
	cch_move_t move;
	cch_move_t reply;
	unsigned char nextlen;
	struct puzzle_step_s* next;
} puzzle_step_t;

typedef struct {
	char fen[SAFE_FEN_LENGTH];
	puzzle_step_t root;
	unsigned char num_variations;
	unsigned char min_depth;
	unsigned char max_depth;
	unsigned char start_material;
	char start_material_diff;
	unsigned char end_material_min;
	unsigned char end_material_max;
	char end_material_diff_min;
	char end_material_diff_max;
	struct {
		bool checkmate:1;
		unsigned char checkmate_piece:6;
		bool checkmate_smothered:1;
		bool checkmate_suffocation:1;
		bool checkmate_back_rank:1;
		bool checkmate_bodens:1;
		bool checkmate_grecos:1;
		bool stalemate:1;
		bool threefold:1;
		bool perpetual:1;
		bool mate_threat:1;
		bool discovered_attack:1;
		bool discovered_check:1;
		bool double_check:1;
		bool promotion:1;
		bool underpromotion:1;
		bool pin:1;
		bool fork:1;
		bool skewer:1;
		bool winning_position:1;
		bool drawing_position:1;
		bool endgame:1;
		bool endgame_q:1;
		bool endgame_r:1;
		bool endgame_b:1;
		bool endgame_n:1;
		bool endgame_p:1;
		bool endgame_m:1;
		bool endgame_M:1;
		bool endgame_Mm:1;
		bool undermining:1;
		bool deflection:1;
		bool trapped_piece:1;
		bool overloaded_piece:1;
	} tags;
} puzzle_t;

unsigned char puzzle_consider(const uci_eval_t*, unsigned char, puzzlegen_settings_t, unsigned char);
void puzzle_free(puzzle_t*);
void puzzle_init(puzzle_t*, const cch_board_t*, const cch_move_t*);
void puzzle_print(const puzzle_t*);
void puzzle_build(const uci_engine_context_t*, puzzle_t*, cch_board_t*, const char*, puzzlegen_settings_t);
void puzzle_finalize(puzzle_t*, cch_board_t*);



/* ----- load.c ----- */

int puzzle_load(puzzle_t*, const char*);



/* ----- tags.c ----- */

void tags_print(const puzzle_t*);
void tags_puzzle(puzzle_t*, cch_board_t*);



/* ----- eval.c ----- */

void eval_material(const cch_board_t*, bool, unsigned char*, char*);
char eval_quiet_material(cch_board_t*, char, char);



/* ----- gumbleextras.c ----- */

void cche_play_null_move(cch_board_t*, cch_move_t*, cch_undo_move_state_t*);
bool cche_moves_through_square(const cch_move_t*, cch_square_t);
unsigned char cche_own_takers_of_square(cch_board_t*, cch_square_t, cch_movelist_t, cch_move_legality_t);
unsigned char cche_enemy_takers_of_square(cch_board_t*, cch_square_t, cch_movelist_t, cch_move_legality_t);
bool cche_could_take(cch_board_t*, cch_square_t, cch_square_t, cch_move_legality_t);

#endif
