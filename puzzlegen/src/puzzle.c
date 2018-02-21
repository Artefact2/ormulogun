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

unsigned char puzzle_consider(const uci_eval_t* evals, unsigned char nlines, puzzlegen_settings_t s, unsigned char depth) {
	/* No moves or forced move? */
	if(nlines < 2) return nlines == 1 && depth > 0;

	/* Clearly lost position? */
	if((evals[0].type == SCORE_MATE && evals[0].score < 0)
	   || (evals[0].type == SCORE_CP && evals[0].score < -s.eval_cutoff)) return 0;

	if(depth == 0) {
		/* Clearly won position? */
		if(evals[nlines - 1].type == SCORE_MATE && evals[nlines - 1].score > 0) return 0;
		if(evals[nlines - 1].type == SCORE_CP && evals[nlines - 1].score > s.eval_cutoff) return 0;
	}

	/* Forced mate? */
	if(evals[0].type == SCORE_MATE) {
		unsigned char i = 1;
		while(i < nlines && evals[i].type == SCORE_MATE && evals[i].score == evals[0].score) {
			++i;
		}
		return i < nlines;
	}

	unsigned int diff = 0;
	int cutoff = 0;
	unsigned char i;
	for(i = 1; i < nlines && evals[i].type == SCORE_CP; ++i) {
		if(evals[i - 1].score - evals[i].score > diff) {
			diff = evals[i - 1].score - evals[i].score;
			cutoff = evals[i].score;
		}
	}

	if(diff < s.puzzle_threshold_absolute) {
		if(i < nlines) return i; /* Escape forced mate */

		if(evals[0].score - evals[i - 1].score < s.puzzle_threshold_absolute) {
			return 0;
		} else {
			cutoff = evals[0].score - s.puzzle_threshold_absolute;
		}
	}

	for(i = 1; (float)(evals[0].score - evals[i].score) / (float)(evals[0].score - cutoff) < s.variation_cutoff_relative; ++i);
	return i;
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
	char lan[SAFE_ALG_LENGTH];

	cch_format_lan_move(&(st->reply), lan, SAFE_ALG_LENGTH);
	printf("[\"%s\",{", lan);

	for(unsigned char i = 0; i < st->nextlen; ++i) {
		if(i > 0) putchar(',');
		cch_format_lan_move(&(st->next[i].move), lan, SAFE_ALG_LENGTH);
		printf("\"%s\":", lan);
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

void puzzle_init(puzzle_t* p, const cch_board_t* b, const cch_move_t* m) {
	cch_return_t ret;
	ret = cch_save_fen(b, p->fen, SAFE_FEN_LENGTH);
	assert(ret == CCH_OK);
	p->root.reply = *m;
	eval_material(b, true, &(p->start_material), &(p->start_material_diff));
}

static void puzzle_finalize_step(puzzle_t* p, puzzle_step_t* st, unsigned char depth) {
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
	uci_eval_t evals[s.max_variations + 1];
	cch_undo_move_state_t um, ur;

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
	if((i = puzzle_consider(evals, nlines, s, depth)) == 0) {
		/* Puzzle over, after computer reply of last puzzle move */

		unsigned char total;
		char diff;

		eval_material(b, false, &total, &diff); /* XXX: assuming a quiet position */
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

	st->nextlen = i;
	st->next = malloc(i * sizeof(puzzle_step_t));

	for(i = 0; i < st->nextlen; ++i) {
		cch_parse_lan_move(evals[i].bestlan, &(st->next[i].move));
		cch_play_legal_move(b, &(st->next[i].move), &um);

		if(b->smoves) {
			fill_threefold_entry(&(tftable[tftlen]), b);
			if(check_threefold(p, b, tftable, tftlen)) {
				puzzle_finalize_step(p, &(st->next[i]), depth + 1);
				cch_undo_move(b, &(st->next[i].move), &um);
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
			cch_undo_move(b, &(st->next[i].move), &um);
			continue;
		}

		cch_parse_lan_move(evals[s.max_variations].bestlan, &(st->next[i].reply));
		cch_play_legal_move(b, &(st->next[i].reply), &ur);

		if(b->smoves <= 1) {
			threefold_entry_t tftable[s.max_depth << 1];
			fill_threefold_entry(tftable, b);
			puzzle_build_step(ctx, depth + 1, p, &(st->next[i]), b, engine_limiter, s, tftable, b->smoves);
		} else {
			fill_threefold_entry(&(tftable[tftlen + 1]), b);
			puzzle_build_step(ctx, depth + 1, p, &(st->next[i]), b, engine_limiter, s, tftable, tftlen + 2);
		}

		cch_undo_move(b, &(st->next[i].reply), &ur);
		cch_undo_move(b, &(st->next[i].move), &um);
	}
}

/* Filter simple 2-ply trades with no choices involved */
static bool puzzle_is_trivial(puzzle_t* p, cch_board_t* b) {
	if(p->min_depth != 1) return false;

	/* Last move was not a capture */
	if(!CCH_GET_SQUARE(b, p->root.reply.end)) return false;

	for(unsigned char i = 0; i < p->root.nextlen; ++i) {
		if(p->root.next[i].move.end != p->root.reply.end) {
			/* Puzzle move is not a takeback */
			return false;
		}
	}

	cch_movelist_t ml;
	unsigned char takebacks = 0, stop = cch_generate_moves(b, ml, CCH_LEGAL, 0, 64);
	for(unsigned char i = 0; i < stop; ++i) {
		if(ml[i].end == p->root.reply.end) ++takebacks;
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

	/* XXX: prune branches that lose on material gains */

	tags_puzzle(p, b, ctx);
}
