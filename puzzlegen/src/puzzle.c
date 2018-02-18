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

#include "gen-puzzles.h"
#include <string.h>
#include <stdbool.h>
#include <assert.h>

typedef struct {
	cch_hash_t h;
	bool check;
} threefold_entry_t;

bool puzzle_consider(const uci_eval_t* evals, unsigned char nlines, puzzlegen_settings_t s, unsigned char depth) {
	/* No moves or forced move? */
	if(nlines < 2) return nlines == 1 && depth > 0;

	/* Clearly lost position? */
	if((evals[0].type == SCORE_MATE && evals[0].score < 0)
	   || (evals[0].type == SCORE_CP && evals[0].score < -s.eval_cutoff)) return false;

	if(depth == 0) {
		/* Clearly won position? */
		if(evals[nlines - 1].type == SCORE_MATE && evals[nlines - 1].score > 0) return false;
		if(evals[nlines - 1].type == SCORE_CP && evals[nlines - 1].score > s.eval_cutoff) return false;
	}

	/* Forced mate? */
	if(evals[0].type == SCORE_MATE) {
		if(evals[nlines - 1].type == SCORE_CP) return true;
		if(evals[nlines - 1].score != evals[0].score) {
			return evals[0].score + depth <= s.max_depth;
		}
		return false;
	}

	/* Escape mate threat? */
	if(evals[0].type == SCORE_CP && evals[nlines - 1].type == SCORE_MATE) {
	    /* XXX: needs refinement */
		return s.best_eval_cutoff_start < 100000;
	}

	assert(evals[0].type == SCORE_CP && evals[nlines - 1].type == SCORE_CP);
	return evals[0].score - evals[nlines - 1].score > (depth == 0 ? s.best_eval_cutoff_start : s.best_eval_cutoff_continue);
}

static void puzzle_free_steps(puzzle_step_t* st) {
	for(unsigned char i = 0; i < st->nextlen; ++i) {
		puzzle_free_steps(&(st->next[i]));
	}

	free(st->next);
	st->nextlen = 0;
	st->next = 0;
}

void puzzle_free(puzzle_t* p) {
	puzzle_free_steps(&(p->root));
}

static void puzzle_print_step(const puzzle_step_t* st) {
	printf("[\"%s\",{", st->reply);

	for(unsigned char i = 0; i < st->nextlen; ++i) {
		if(i > 0) putchar(',');
		printf("\"%s\":", st->next[i].move);
		if(st->next[i].nextlen == 0) {
			putchar('0');
		} else {
			puzzle_print_step(&(st->next[i]));
		}
	}

	fputs("}]", stdout);
}

void puzzle_print(const puzzle_t* p) {
	printf("[\"%s\",", p->fen);
	puzzle_print_step(&(p->root));
	putchar(',');
	tags_print(p);
	puts("]");
	fflush(stdout); /* XXX: play nice with xargs? */
}

static void count_material(const cch_board_t* b, bool reverse, unsigned char* total, char* diff) {
	static const char mat[] = { 0, 1, 3, 3, 5, 9, 0 };
	cch_piece_t p;

	*total = 0;
	*diff = 0;

	for(unsigned char sq = 0; sq < 64; ++sq) {
		p = CCH_GET_SQUARE(b, sq);
		*total += mat[CCH_PURE_PIECE(p)];
		if(CCH_IS_OWN_PIECE(b, p) ^ reverse) {
			*diff += mat[CCH_PURE_PIECE(p)];
		} else {
			*diff -= mat[CCH_PURE_PIECE(p)];
		}
	}
}

void puzzle_init(puzzle_t* p, const cch_board_t* b) {
	cch_return_t ret;
	ret = cch_save_fen(b, p->fen, SAFE_FEN_LENGTH);
	assert(ret == CCH_OK);
	count_material(b, true, &(p->start_material), &(p->start_material_diff));
}

static void puzzle_finalize_step(puzzle_t* p, puzzle_step_t* st, unsigned char depth) {
	st->reply[0] = '\0';
	st->nextlen = 0;
	st->next = 0;

	if(p->min_depth > depth) p->min_depth = depth;
}

static void fill_threefold_entry(threefold_entry_t* e, const cch_board_t* b) {
	e->h = b->hash;
	e->check = CCH_IS_OWN_KING_CHECKED(b);
}

static bool check_threefold(puzzle_t* p, cch_board_t* b, threefold_entry_t* tftable, unsigned char tftlen) {
	if(tftlen >= 3) {
		unsigned char repetitions = 0;

		for(unsigned char i = 0; i < tftlen; ++i) {
			repetitions += tftable[tftlen - 1].h == tftable[i].h;
		}

		if(repetitions >= 3) {
			p->tags.draw = true;
			if(tftable[tftlen - 1].check || tftable[tftlen - 2].check) p->tags.perpetual = true; /* XXX: more complicated than that */
			else p->tags.threefold = true;
			return true;
		}
	}

	return false;
}

static void puzzle_build_step(const uci_engine_context_t* ctx, unsigned char depth,
							  puzzle_t* p, puzzle_step_t* st, cch_board_t* b,
							  const char* engine_limiter, puzzlegen_settings_t s,
							  threefold_entry_t* tftable, unsigned char tftlen) {
	unsigned char nlines, i;
	cch_undo_move_state_t umove, ureply;
	cch_move_t m, mr;
	uci_eval_t evals[s.max_variations + 1];

	if(depth > s.max_depth || p->min_depth == 0) {
		/* Puzzle is too long */
		p->min_depth = 0;
		return;
	}

	if(check_threefold(p, b, tftable, tftlen)) {
		puzzle_finalize_step(p, st, depth);
		return;
	}

	nlines = uci_eval(ctx, engine_limiter, b, evals, s.max_variations + 1);
	if(!puzzle_consider(evals, nlines, s, depth)) {
		/* Puzzle over, after computer reply of last puzzle move */

		unsigned char total;
		char diff;

		count_material(b, false, &total, &diff); /* XXX: assuming a quiet position */
		if(diff < p->end_material_diff_min) p->end_material_diff_min = diff;
		if(total < p->end_material_min) p->end_material_min = total;

		puzzle_finalize_step(p, st, depth);

		if(evals[0].type == SCORE_MATE && evals[0].score > 0) {
			/* Puzzle leads to forced checkmate */
			p->tags.checkmate = true;
			p->checkmate_length = 0; /* XXX: get rid of this and use winning_position flag in struct */
		}
		return;
	}

	if(evals[0].type == SCORE_MATE) {
		for(i = 0; i < nlines && evals[i].type == SCORE_MATE && evals[i].score == evals[0].score; ++i);
	} else {
		for(i = 0; i < nlines && evals[i].type == SCORE_CP && evals[0].score - evals[i].score < s.variation_eval_cutoff; ++i);
	}
	st->nextlen = i;
	st->next = malloc(i * sizeof(puzzle_step_t));

	for(i = 0; i < st->nextlen; ++i) {
		strncpy(st->next[i].move, evals[i].bestlan, SAFE_ALG_LENGTH);
		cch_parse_lan_move(evals[i].bestlan, &m);
		cch_play_legal_move(b, &m, &umove);

		if(b->smoves) {
			fill_threefold_entry(&(tftable[tftlen]), b);
			if(check_threefold(p, b, tftable, tftlen)) {
				puzzle_finalize_step(p, &(st->next[i]), depth + 1);
				cch_undo_move(b, &m, &umove);
				continue;
			}
		}

		if(uci_eval(ctx, engine_limiter, b, &(evals[s.max_variations]), 1) == 0) {
			/* Game over */
			if(CCH_IS_OWN_KING_CHECKED(b)) {
				p->tags.checkmate = true;
				p->checkmate_length = depth + 1;
			} else {
				p->tags.stalemate = true;
				p->tags.draw = true;
			}
			puzzle_finalize_step(p, &(st->next[i]), depth + 1);
			cch_undo_move(b, &m, &umove);
			continue;
		}

		tags_after_player_move(ctx, p, b, &m);

		strncpy(st->next[i].reply, evals[s.max_variations].bestlan, SAFE_ALG_LENGTH);
		cch_parse_lan_move(evals[s.max_variations].bestlan, &mr);
		cch_play_legal_move(b, &mr, &ureply);

		if(b->smoves <= 1) {
			threefold_entry_t tftable[s.max_depth << 1];
			fill_threefold_entry(tftable, b);
			puzzle_build_step(ctx, depth + 1, p, &(st->next[i]), b, engine_limiter, s, tftable, b->smoves);
		} else {
			fill_threefold_entry(&(tftable[tftlen + 1]), b);
			puzzle_build_step(ctx, depth + 1, p, &(st->next[i]), b, engine_limiter, s, tftable, tftlen + 2);
		}

		cch_undo_move(b, &mr, &ureply);
		cch_undo_move(b, &m, &umove);
	}
}

static bool puzzle_is_trivial(puzzle_t* p, cch_board_t* b) {
	if(p->min_depth != 1) return false;

	/* Filter simple 2-ply trades with no choices involved */
	cch_move_t m;
	cch_parse_lan_move(p->root.reply, &m);
	if(!CCH_GET_SQUARE(b, m.end)) return false;

	cch_movelist_t ml;
	unsigned char i, takebacks = 0, stop = cch_generate_moves(b, ml, CCH_LEGAL, 0, 64);
	for(i = 0; i < stop; ++i) {
		if(ml[i].end == m.end) ++takebacks;
	}
	return p->root.nextlen == takebacks;
}

void puzzle_build(const uci_engine_context_t* ctx, puzzle_t* p, cch_board_t* b, const char* engine_limiter, puzzlegen_settings_t s) {
	p->min_depth = 255;
	p->end_material_min = 255;
	p->end_material_diff_min = 127;
	memset(&(p->tags), 0, sizeof(p->tags));

	threefold_entry_t tftable[s.max_depth << 1];
	puzzle_build_step(ctx, 0, p, &(p->root), b, engine_limiter, s, tftable, 0);

	if(puzzle_is_trivial(p, b)) {
		p->min_depth = 0;
		return;
	}

	tags_after_puzzle_done(p);
}
