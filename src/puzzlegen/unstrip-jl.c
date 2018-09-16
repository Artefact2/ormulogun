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
#include "config.h"
#include <stdio.h>

static void fill_leaves(cch_board_t* b, puzzle_t* p, puzzle_step_t* st, uci_engine_context_t* ctx) {
	if(st->nextlen > 0) {
		/* Not a leaf; traverse into children */

		cch_undo_move_state_t um, ur;
		cch_play_legal_move(b, &(st->move), &um);
		cch_play_legal_move(b, &(st->reply), &ur);

		for(unsigned char i = 0; i < st->nextlen; ++i) {
			fill_leaves(b, p, &(st->next[i]), ctx);
		}

		cch_undo_move(b, &(st->reply), &ur);
		cch_undo_move(b, &(st->move), &um);
		return;
	}

	/* Leaf: fill reply from engine if blank */

	if(st->reply.start != 255) {
		/* Already have reply */
		return;
	}

	cch_undo_move_state_t u;
	uci_eval_t eval;
	cch_play_legal_move(b, &(st->move), &u);

	/* XXX: somewhat duplicate code with puzzle.c */
	/* XXX: perpetual check/threefold tagging */

	if(uci_eval(ctx, uci_limiter, b, &eval, 1) > 0) {
		cch_parse_lan_move(eval.bestlan, &(st->reply));

		if(eval.type == SCORE_MATE && eval.score > 0) {
			p->tags.winning_position = true;
		} else if(eval.type == SCORE_CP && eval.score == 0) {
			p->tags.drawing_position = true;
		}
	} else {
		if(CCH_IS_OWN_KING_CHECKED(b)) {
			p->tags.checkmate = true;
		} else {
			p->tags.stalemate = true;
		}
	}

	cch_undo_move(b, &(st->move), &u);
}

int main(void) {
	cch_board_t b;
	puzzle_t p;
	uci_engine_context_t ctx;

	if(uci_create(uci_engine, &ctx)) {
		perror("could not spawn engine");
		return 1;
	}
	uci_init(&ctx, uci_engine_opts);

	char* line = 0;
	size_t linelen;

	while(getline(&line, &linelen, stdin) != -1) {
		if(puzzle_load(&p, line)) {
			fprintf(stderr, "Parse error loading puzzle: %s", line);
			return 2;
		}

		cch_load_fen(&b, p.fen);
		puzzle_init(&p, &b, &(p.root.reply));
		cch_play_legal_move(&b, &(p.root.reply), 0);
		for(unsigned char i = 0; i < p.root.nextlen; ++i) {
			fill_leaves(&b, &p, &(p.root.next[i]), &ctx);
		}
		tags_puzzle(&p, &b);
		puzzle_print(&p);
		puzzle_free(&p);
	}

	return 0;
}
