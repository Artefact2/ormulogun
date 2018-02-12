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

	if(!p->tags.draw && !p->tags.checkmate) {
		MAYBE_PRINT_TAG(p->tags.mate_threat, "Checkmate threat");
		MAYBE_PRINT_TAG(p->end_material_diff_min > p->start_material_diff, "Material gain");
		MAYBE_PRINT_TAG(p->end_material_diff_min == p->start_material_diff && p->end_material_min < p->start_material, "Trade");
		MAYBE_PRINT_TAG(p->end_material_diff_min == p->start_material_diff && p->end_material_min == p->start_material, "Quiet");
	}

	putchar(']');
}

static void tags_mate_threat(const uci_engine_context_t* ctx, puzzle_t* p, char* ll, size_t lllen) {
	if(p->tags.mate_threat) return;

	uci_eval_t ev;

	strcpy(ll + lllen, " null");
	lllen += 5;
	if(uci_eval(ctx, "nodes 1000000", ll, &ev, 1)) { /* XXX */
		if(ev.type == SCORE_MATE && ev.score > 0) {
			p->tags.mate_threat = true;
		}
	}
}

void tags_after_player_move(const uci_engine_context_t* ctx, puzzle_t* p, char* ll, size_t lllen) {
	tags_mate_threat(ctx, p, ll, lllen);
}
