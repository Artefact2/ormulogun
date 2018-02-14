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
#ifndef ORMULOGUN_GEN_PUZZLES_H
#define ORMULOGUN_GEN_PUZZLES_H

#include <engine.h>
#include <stdio.h>
#include <stdbool.h>

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
unsigned char uci_eval(const uci_engine_context_t*, const char*, const char*, uci_eval_t*, unsigned char);



/* ----- puzzle.c ----- */

typedef struct {
	int eval_cutoff;
	int best_eval_cutoff_start;
	int best_eval_cutoff_continue;
	int variation_eval_cutoff;
	unsigned int min_ply;
	unsigned char max_variations;
	unsigned char max_depth;
} puzzlegen_settings_t;

typedef struct puzzle_step_s {
	char move[SAFE_ALG_LENGTH];
	char reply[SAFE_ALG_LENGTH];
	unsigned char nextlen;
	struct puzzle_step_s* next;
} puzzle_step_t;

typedef struct {
	char fen[SAFE_FEN_LENGTH];
	puzzle_step_t root;
	unsigned char min_depth;
	unsigned char checkmate_length;
	unsigned char start_material;
	char start_material_diff;
	unsigned char end_material_min;
	char end_material_diff_min;
	struct {
		bool checkmate:1;
		bool stalemate:1;
		bool draw:1;
		bool escape_mate:1;
		bool mate_threat:1;
		bool discovered_check:1;
		bool double_check:1;
	} tags;
} puzzle_t;

bool puzzle_consider(const uci_eval_t*, unsigned char, puzzlegen_settings_t, unsigned char);
void puzzle_free(puzzle_t*);
void puzzle_init(puzzle_t*, const cch_board_t*);
void puzzle_print(const puzzle_t*);
void puzzle_build(const uci_engine_context_t*, char*, size_t, puzzle_t*, cch_board_t*, const char*, puzzlegen_settings_t);



/* ----- tags.c ----- */

void tags_print(const puzzle_t*);
void tags_after_player_move(const uci_engine_context_t*, puzzle_t*, char*, size_t, cch_board_t*, const cch_move_t*);

#endif
