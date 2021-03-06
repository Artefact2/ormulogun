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

#include "ormulogun.h"
#include <string.h>
#include <stdbool.h>
#include <assert.h>

typedef struct {
	cch_hash_t h;
	bool check;
} threefold_entry_t;

/* Returns 255 if current puzzle should be discarded, otherwise number
 * of possible moves for next puzzle step. */
unsigned char puzzle_consider(const uci_eval_t* evals, unsigned char nlines, puzzlegen_settings_t s, unsigned char depth) {
	if(nlines == 0) {
		/* XXX */
		return 255;
	}

	if(nlines == 1) {
		/* Accept forced moves mid-puzzle */
		return depth == 0 ? 255 : 1;
	}

	/* Clearly lost position? */
	if((evals[0].type == SCORE_MATE && evals[0].score < 0)
	   || (evals[0].type == SCORE_CP && evals[0].score < s.min_eval_cutoff)) return 255;

	/* Clearly won position? */
	if(depth == 0) {
		if(evals[nlines - 1].type == SCORE_MATE && evals[nlines - 1].score > 0) return 255;
		if(evals[nlines - 1].type == SCORE_CP && evals[nlines - 1].score > s.max_eval_cutoff) return 255;
	}

	unsigned char i, j;

	/* Forced mate? */
	if(evals[0].type == SCORE_MATE) {
		if(evals[0].score > s.max_depth) {
			/* Accept all forced mates, no matter the length (SF in
			 * particular is unreliable for long checkmate sequences) */
			for(i = 1; i < nlines && evals[i].type == SCORE_MATE && evals[i].score > 0; ++i);
		} else {
			/* Short mate, depths are reliable, go for the shortest */
			for(i = 1; i < nlines && evals[i].type == SCORE_MATE && evals[i].score == evals[0].score; ++i);
		}

		if(i == nlines) {
			/* More moves may lead to forced or best checkmate, unclear, end puzzle here */
			return 0;
		}

		return i;
	}

	for(i = 1; evals[i].type == SCORE_CP && i < nlines; ++i) {
		/* Do we have a tactical opportunity? */
		if(evals[0].score - evals[i].score < s.puzzle_threshold_absolute) continue;
		int cutoff = evals[0].score - (evals[0].score - evals[i].score) * s.variation_cutoff_relative;
		for(j = 1; j < i; ++j) {
			if(evals[j].score < cutoff) {
				/* Tactical opportunity, but variations are too spread out, abort puzzle */
				return 255;
			}
		}
		return i;
	}

	if(nlines < s.max_variations) {
		/* XXX */
		return 255;
	}

	/* Normal puzzle end; no tactical opportunities anymore */
	if(i == nlines) return 0;

	/* Avoid forced mate iff mate-avoiding moves are within variation limits */
	int cutoff = evals[0].score - s.puzzle_threshold_absolute * s.variation_cutoff_relative;
	for(j = 1; j < i; ++j) {
		if(evals[j].score < cutoff) return 255;
	}
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
			if(st->next[i].reply.start == 255) {
				fputs("\"\"", stdout);
			} else {
				cch_format_lan_move(&(st->next[i].reply), lan, SAFE_ALG_LENGTH);
				printf("\"%s\"", lan);
			}
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
}

void puzzle_init(puzzle_t* p, const cch_board_t* b, const cch_move_t* m) {
	cch_return_t ret;
	ret = cch_save_fen(b, p->fen, SAFE_FEN_LENGTH);
	assert(ret == CCH_OK);
	p->min_depth = 255;
	p->root.reply = *m;
	eval_material(b, true, &(p->start_material), &(p->start_material_diff));
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

	st->nextlen = 0;
	st->next = 0;

	if(depth > s.max_depth || p->min_depth == 0) {
		/* Puzzle is too long or was aborted earlier */
		p->min_depth = 0;
		return;
	}

	if(check_threefold(p, b, tftable, tftlen)) return;

	nlines = uci_eval(ctx, engine_limiter, b, evals, s.max_variations + 1);
	i = puzzle_consider(evals, nlines, s, depth);
	if(i == 255) {
		/* Abort puzzle */
		p->min_depth = 0;
		return;
	}
	if(i == 0) {
		/* Puzzle over, after computer reply of last puzzle move */
		if(evals[0].type == SCORE_MATE && evals[0].score > 0) {
			/* Puzzle leads to forced checkmate */
			p->tags.winning_position = true;
		} else if(evals[0].type == SCORE_CP && evals[0].score == 0) {
			p->tags.drawing_position = true;
		}
		return;
	}

	st->nextlen = i;
	st->next = malloc(i * sizeof(puzzle_step_t));

	for(i = 0; i < st->nextlen; ++i) {
		st->next[i].nextlen = 0;
		st->next[i].next = 0;
		st->next[i].reply.start = 255;

		cch_parse_lan_move(evals[i].bestlan, &(st->next[i].move));
		cch_play_legal_move(b, &(st->next[i].move), &um);

		if(b->smoves) {
			fill_threefold_entry(&(tftable[tftlen]), b);
			if(check_threefold(p, b, tftable, tftlen)) {
				cch_undo_move(b, &(st->next[i].move), &um);
				continue;
			}
		}

		if(uci_eval(ctx, engine_limiter, b, &(evals[s.max_variations]), 1) == 0) {
			/* Game over */
			if(CCH_IS_OWN_KING_CHECKED(b)) {
				p->tags.checkmate = true;
			} else {
				p->tags.stalemate = true;
			}
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

static bool puzzle_is_trivial(const puzzle_t* p, const cch_board_t* b) {
	return p->min_depth < 2 && !p->tags.checkmate;
}

static bool puzzle_prune(puzzle_step_t* st, cch_board_t* b) {
	/* Prune children first */
	for(unsigned char i = 0; i < st->nextlen && st->next[i].nextlen; ++i) {
		cch_undo_move_state_t um, ur;
		bool should_prune;

		cch_play_move(b, &st->next[i].move, &um);
		cch_play_move(b, &st->next[i].reply, &ur);
		should_prune = puzzle_prune(&st->next[i], b);
		cch_undo_move(b, &st->next[i].reply, &ur);
		cch_undo_move(b, &st->next[i].move, &um);

		if(should_prune) {
			puzzle_free_steps(&st->next[i]);
			st->next[i] = st->next[st->nextlen - 1];
			--st->nextlen;
			--i;
		}
	}

	/* Are we in a node with only leaf children? */
	for(unsigned char i = 0; i < st->nextlen; ++i) {
		if(st->next[i].nextlen > 0) return false;
	}

	/* Are these moves forced? */
	static cch_movelist_t ml;
	unsigned char stop = cch_generate_moves(b, ml, CCH_LEGAL, 0, 64);
	assert(stop >= st->nextlen);
	return stop == st->nextlen;
}

void puzzle_finalize(puzzle_t* p, cch_board_t* b) {
	if(p->min_depth == 0) {
		return;
	}

	puzzle_prune(&p->root, b);
	tags_puzzle(p, b);

	if(puzzle_is_trivial(p, b)) {
		p->min_depth = 0;
	}
}

void puzzle_build(const uci_engine_context_t* ctx, puzzle_t* p, cch_board_t* b, const char* engine_limiter, puzzlegen_settings_t s) {
	memset(&(p->tags), 0, sizeof(p->tags));

	threefold_entry_t tftable[(s.max_depth + 1) << 1];
	puzzle_build_step(ctx, 0, p, &(p->root), b, engine_limiter, s, tftable, 0);

	puzzle_finalize(p, b);
}
