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

#define MAYBE_PRINT_TAG(f, n) do {				\
		if(f) {									\
			printf(",\"%s\"", (n));				\
		}										\
	} while(0)

void tags_print(const puzzle_t* p) {
	printf("[\"Depth %d\"", p->min_depth);

	if(p->tags.checkmate) {
		printf(",\"Checkmate\",\"Checkmate in %d\"", p->checkmate_length);
	}

	MAYBE_PRINT_TAG(p->tags.draw, "Draw");
	MAYBE_PRINT_TAG(p->tags.stalemate, "Draw (Stalemate)");
	MAYBE_PRINT_TAG(p->tags.escape_mate, "Escape checkmate");
	MAYBE_PRINT_TAG(p->tags.discovered_check, "Discovered check");
	MAYBE_PRINT_TAG(p->tags.double_check, "Double check");
	MAYBE_PRINT_TAG(p->tags.promotion, "Promotion");
	MAYBE_PRINT_TAG(p->tags.underpromotion, "Underpromotion");

	if(!p->tags.draw && !p->tags.checkmate) {
		MAYBE_PRINT_TAG(p->tags.mate_threat, "Checkmate threat");
		MAYBE_PRINT_TAG(p->end_material_diff_min > p->start_material_diff, "Material gain");
		MAYBE_PRINT_TAG(p->end_material_diff_min == p->start_material_diff && p->end_material_min < p->start_material, "Trade");
		MAYBE_PRINT_TAG(p->end_material_diff_min == p->start_material_diff && p->end_material_min == p->start_material, "Quiet");
	}

	putchar(']');
}

/* XXX */
static void tags_mate_threat(const uci_engine_context_t* ctx, puzzle_t* p, cch_board_t* b) {
	if(p->tags.mate_threat) return;

	uci_eval_t ev;

	b->side = !b->side;
	if(uci_eval(ctx, "nodes 1000000", b, &ev, 1)) { /* XXX */
		if(ev.type == SCORE_MATE && ev.score > 0) {
			p->tags.mate_threat = true;
		}
	}
	b->side = !b->side;
}

static void tags_discovered_double_check(puzzle_t* p, cch_board_t* b, const cch_move_t* last) {
	if(p->tags.discovered_check && p->tags.double_check) return;

	cch_movelist_t ml;
	unsigned char i, checkers = 0, stop;

	b->side = !b->side;
	stop = cch_generate_moves(b, ml, CCH_LEGAL, 0, 64);
	b->side = !b->side;

	for(i = 0; i < stop; ++i) {
		if(ml[i].end == CCH_OWN_KING(b)) {
			if(ml[i].start != last->end) {
				p->tags.discovered_check = true;
			}
			++checkers;
		}
	}

	if(checkers > 1) p->tags.double_check = true;
}

void tags_after_player_move(const uci_engine_context_t* ctx, puzzle_t* p, cch_board_t* b, const cch_move_t* last) {
	if(CCH_IS_OWN_KING_CHECKED(b)) {
		tags_discovered_double_check(p, b, last);
	} else {
		tags_mate_threat(ctx, p, b);
	}

	if(last->promote) {
		p->tags.promotion = true;

		if(last->promote != CCH_QUEEN) {
			p->tags.underpromotion = true;
		}
	}
}
