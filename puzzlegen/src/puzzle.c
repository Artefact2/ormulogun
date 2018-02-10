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

bool puzzle_consider(const uci_eval_t* evals, unsigned char nlines, int eval_cutoff, int best_eval_cutoff) {
	/* No moves or forced move? */
	if(nlines < 2) return false;

	/* Clearly lost position? */
	if((evals[0].type == SCORE_MATE && evals[0].score < 0)
	   || (evals[0].type == SCORE_CP && evals[0].score < -eval_cutoff)) return false;

	/* Clearly won position? */
	if((evals[nlines - 1].type == SCORE_MATE && evals[nlines - 1].score > 0)
	   || (evals[nlines - 1].type == SCORE_CP && evals[nlines - 1].score > eval_cutoff)) return false;

	/* Force mate, or avoid forced mate */
	if(evals[0].type != evals[nlines - 1].type) return /*true*/ false; /* XXX: todo */

	if(evals[0].type == SCORE_MATE && evals[nlines - 1].type == SCORE_MATE) {
		return false; /* XXX: todo */
		return evals[0].score * evals[nlines - 1].type < 0;
	}

	return evals[0].score - evals[nlines - 1].score > best_eval_cutoff;
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

void puzzle_print(puzzle_t* p) {
	printf("[\"%s\",", p->fen);
	puzzle_print_step(&(p->root));
	puts("]");
	fflush(stdout); /* Play nice with xargs */
}

static void puzzle_build_step(const uci_engine_context_t* ctx, char* lanlist, size_t lanlistlen,
							  puzzle_step_t* st, cch_board_t* b,
							  const char* engine_limiter,
							  int max_variations, int eval_cutoff, int best_eval_cutoff, int variation_eval_cutoff) {
	unsigned char nlines, nreplies, i;
	size_t nll;
	cch_undo_move_state_t umove, ureply;
	cch_move_t m, mr;
	uci_eval_t evals[max_variations + 1];

	nlines = uci_eval(ctx, engine_limiter, lanlist, evals, max_variations + 1);
	if(!puzzle_consider(evals, nlines, eval_cutoff, best_eval_cutoff)) {
		st->reply[0] = '\0';
		st->nextlen = 0;
		st->next = 0;
		return;
	}

	assert(evals[0].type == SCORE_CP && evals[nlines - 1].type == SCORE_CP);
	assert(evals[0].score - evals[nlines - 1].score > best_eval_cutoff);

	for(i = 0; i < nlines; ++i) {
		if(evals[0].score - evals[i].score < variation_eval_cutoff) continue;
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

		nreplies = uci_eval(ctx, engine_limiter, lanlist, &(evals[max_variations]), 1);
		if(nreplies == 0) {
			/* Game over */
			st->next[i].reply[0] = '\0';
			st->next[i].nextlen = 0;
			st->next[i].next = 0;
			continue;
		}

		lanlist[nll] = ' ';
		++nll;
		strncpy(st->next[i].reply, evals[max_variations].bestlan, SAFE_ALG_LENGTH);
		strncpy(lanlist + nll, evals[max_variations].bestlan, SAFE_ALG_LENGTH);
		nll += strlen(evals[max_variations].bestlan);

		cch_parse_lan_move(evals[i].bestlan, &m);
		cch_play_legal_move(b, &m, &umove);
		cch_parse_lan_move(evals[max_variations].bestlan, &mr);
		cch_play_legal_move(b, &mr, &ureply);

		puzzle_build_step(ctx, lanlist, nll,
						  &(st->next[i]), b,
						  engine_limiter, max_variations, eval_cutoff, best_eval_cutoff, variation_eval_cutoff);

		cch_undo_move(b, &mr, &ureply);
		cch_undo_move(b, &m, &umove);
	}
}

void puzzle_build(const uci_engine_context_t* ctx, char* lanlist, size_t lanlistlen,
				  puzzle_t* p, cch_board_t* b,
				  const char* engine_limiter,
				  int max_variations, int eval_cutoff, int best_eval_cutoff, int variation_eval_cutoff) {
	puzzle_build_step(ctx, lanlist, lanlistlen,
					  &(p->root), b,
					  engine_limiter, max_variations, eval_cutoff, best_eval_cutoff, variation_eval_cutoff);
}
