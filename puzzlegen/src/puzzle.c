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

bool puzzle_consider(const uci_eval_t* evals, unsigned char nlines, puzzlegen_settings_t s) {
	/* No moves or forced move? */
	if(nlines < 2) return false;

	/* Clearly lost position? */
	if((evals[0].type == SCORE_MATE && evals[0].score < 0)
	   || (evals[0].type == SCORE_CP && evals[0].score < -s.eval_cutoff)) return false;

	/* Clearly won position? */
	if((evals[nlines - 1].type == SCORE_MATE && evals[nlines - 1].score > 0)
	   || (evals[nlines - 1].type == SCORE_CP && evals[nlines - 1].score > s.eval_cutoff)) return false;

	/* Force mate, or avoid forced mate */
	if(evals[0].type != evals[nlines - 1].type) return /*true*/ false; /* XXX: todo */

	if(evals[0].type == SCORE_MATE && evals[nlines - 1].type == SCORE_MATE) {
		return false; /* XXX: todo */
		return evals[0].score * evals[nlines - 1].type < 0;
	}

	return evals[0].score - evals[nlines - 1].score > s.best_eval_cutoff;
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

static void puzzle_print_step(puzzle_step_t* st) {
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

#define MAYBE_PRINT_TAG(f, n) do {				\
		if(f) {									\
			printf(",\"%s\"", n);				\
		}										\
	} while(0)

void puzzle_print(puzzle_t* p) {
	if(p->min_depth == 0) return;
	printf("[\"%s\",", p->fen);
	puzzle_print_step(&(p->root));
	printf(",[\"Depth %d\"", p->min_depth);
	MAYBE_PRINT_TAG(p->tags.checkmate, "Checkmate");
	MAYBE_PRINT_TAG(p->tags.draw, "Draw");
	MAYBE_PRINT_TAG(p->tags.stalemate, "Draw: Stalemate");
	puts("]]");
	fflush(stdout); /* XXX: play nice with xargs? */
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
	unsigned char nlines, nreplies, i;
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
	if(!puzzle_consider(evals, nlines, s)) {
		/* Puzzle over */
		puzzle_finalize_step(p, st, depth);
		return;
	}

	assert(evals[0].type == SCORE_CP && evals[nlines - 1].type == SCORE_CP);
	assert(evals[0].score - evals[nlines - 1].score > s.best_eval_cutoff);

	for(i = 0; i < nlines; ++i) {
		if(evals[0].score - evals[i].score < s.variation_eval_cutoff) continue;
		st->nextlen = i;
		st->next = malloc(i * sizeof(puzzle_step_t));
		break;
	}

	assert(st->nextlen < nlines);

	for(i = 0; i < st->nextlen; ++i) {
		nll = lanlistlen;
		lanlist[nll] = ' ';
		++nll;
		strncpy(st->next[i].move, evals[i].bestlan, SAFE_ALG_LENGTH);
		strncpy(lanlist + nll, evals[i].bestlan, SAFE_ALG_LENGTH);
		nll += strlen(evals[i].bestlan);
		cch_parse_lan_move(evals[i].bestlan, &m);
		cch_play_legal_move(b, &m, &umove);

		nreplies = uci_eval(ctx, engine_limiter, lanlist, &(evals[s.max_variations]), 1);
		if(nreplies == 0) {
			/* Game over */
			puzzle_finalize_step(p, &(st->next[i]), depth + 1);
			if(CCH_IS_OWN_KING_CHECKED(b)) {
				p->tags.checkmate = true;
			} else {
				p->tags.stalemate = true;
				p->tags.draw = true;
			}
			continue;
		}

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

void puzzle_build(const uci_engine_context_t* ctx, char* lanlist, size_t lanlistlen,
				  puzzle_t* p, cch_board_t* b,
				  const char* engine_limiter, puzzlegen_settings_t s) {
	p->min_depth = 255;
	memset(&(p->tags), 0, sizeof(p->tags));
	puzzle_build_step(ctx, lanlist, lanlistlen, 0, p, &(p->root), b, engine_limiter, s);
}
