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
		return true;
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

static void puzzle_build_step(const uci_engine_context_t* ctx, char* lanlist, size_t lanlistlen, unsigned char depth,
							  puzzle_t* p, puzzle_step_t* st, cch_board_t* b,
							  const char* engine_limiter, puzzlegen_settings_t s) {
	unsigned char nlines, i;
	size_t nll;
	cch_undo_move_state_t umove, ureply;
	cch_move_t m, mr;
	uci_eval_t evals[s.max_variations + 1];

	if(depth > s.max_depth) {
		/* Puzzle is too long */
		p->min_depth = 0;
		return;
	}

	nlines = uci_eval(ctx, engine_limiter, lanlist, evals, s.max_variations + 1);
	if(!puzzle_consider(evals, nlines, s, depth)) {
		/* Puzzle over, after computer reply of last puzzle move */

		unsigned char total;
		char diff;

		count_material(b, false, &total, &diff); /* XXX: assuming a quiet position */
		if(diff < p->end_material_diff_min) p->end_material_diff_min = diff;
		if(total < p->end_material_min) p->end_material_min = total;

		puzzle_finalize_step(p, st, depth);

		if(evals[nlines - 1].type == SCORE_MATE && evals[nlines - 1].score > 0 && depth + evals[0].score <= s.max_depth) {
			/* Incomplete checkmate sequence, because it has too many branches */
			p->tags.checkmate = true;
			p->checkmate_length = depth + evals[0].score; /* XXX? */
		}
		return;
	}

	if(evals[0].type == SCORE_CP && evals[nlines - 1].type == SCORE_MATE && evals[nlines - 1].score < 0) {
		p->tags.escape_mate = true;
	}

	if(evals[0].type == SCORE_MATE) {
		for(i = 0; i < nlines && evals[i].type == SCORE_MATE && evals[i].score == evals[0].score; ++i);
	} else {
		for(i = 0; i < nlines && evals[i].type == SCORE_CP && evals[0].score - evals[i].score < s.variation_eval_cutoff; ++i);
	}
	st->nextlen = i;
	st->next = malloc(i * sizeof(puzzle_step_t));

	for(i = 0; i < st->nextlen; ++i) {
		nll = lanlistlen;
		lanlist[nll] = ' ';
		++nll;
		strncpy(st->next[i].move, evals[i].bestlan, SAFE_ALG_LENGTH);
		strncpy(lanlist + nll, evals[i].bestlan, SAFE_ALG_LENGTH);
		nll += strlen(evals[i].bestlan);
		cch_parse_lan_move(evals[i].bestlan, &m);
		cch_play_legal_move(b, &m, &umove);

		if(uci_eval(ctx, engine_limiter, lanlist, &(evals[s.max_variations]), 1) == 0) {
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

		tags_after_player_move(ctx, p, lanlist, nll, b, &m);

		lanlist[nll] = ' ';
		++nll;
		strncpy(st->next[i].reply, evals[s.max_variations].bestlan, SAFE_ALG_LENGTH);
		strncpy(lanlist + nll, evals[s.max_variations].bestlan, SAFE_ALG_LENGTH);
		nll += strlen(evals[s.max_variations].bestlan);
		cch_parse_lan_move(evals[s.max_variations].bestlan, &mr);
		cch_play_legal_move(b, &mr, &ureply);

		puzzle_build_step(ctx, lanlist, nll, depth + 1, p, &(st->next[i]), b, engine_limiter, s);

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

void puzzle_build(const uci_engine_context_t* ctx, char* lanlist, size_t lanlistlen,
				  puzzle_t* p, cch_board_t* b,
				  const char* engine_limiter, puzzlegen_settings_t s) {
	p->min_depth = 255;
	p->end_material_min = 255;
	p->end_material_diff_min = 127;
	memset(&(p->tags), 0, sizeof(p->tags));

	puzzle_build_step(ctx, lanlist, lanlistlen, 0, p, &(p->root), b, engine_limiter, s);

	if(puzzle_is_trivial(p, b)) {
		p->min_depth = 0;
	}
}
