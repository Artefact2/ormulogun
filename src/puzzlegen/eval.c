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
#include <stdbool.h>

void eval_material(const cch_board_t* b, bool reverse, unsigned char* total, char* diff) {
	static const char mat[] = { 0, 1, 3, 3, 5, 9, 50 };
	cch_piece_t p;

	if(total) *total = 0;
	if(diff) *diff = 0;

	for(unsigned char sq = 0; sq < 64; ++sq) {
		p = CCH_GET_SQUARE(b, sq);
		if(total) *total += mat[CCH_PURE_PIECE(p)];
		if(!diff) continue;
		if(CCH_IS_OWN_PIECE(b, p) ^ reverse) {
			*diff += mat[CCH_PURE_PIECE(p)];
		} else {
			*diff -= mat[CCH_PURE_PIECE(p)];
		}
	}
}

/* This is a very simple α-β search where the eval function is the
 * material difference and the only examined moves are captures. */
char eval_quiet_material(cch_board_t* b, char alpha, char beta) {
	char ev;
	eval_material(b, false, 0, &ev);

	if(ev >= beta) return beta;
	if(alpha < ev) alpha = ev;

	cch_undo_move_state_t undo;
	cch_movelist_t ml;
	unsigned char stop = cch_generate_moves(b, ml, CCH_LEGAL, 0, 64);

	if(stop == 0) {
		if(CCH_IS_OWN_KING_CHECKED(b)) {
			return -127; /* Checkmated */
		} else {
			return 0; /* Stalemated */
		}
	}

	for(unsigned char i = 0; i < stop; ++i) {
		if(!CCH_GET_SQUARE(b, ml[i].end)) continue;
		cch_play_legal_move(b, &(ml[i]), &undo);
		ev = -eval_quiet_material(b, -beta, -alpha);
		cch_undo_move(b, &(ml[i]), &undo);

		if(ev >= beta) return beta;
		if(alpha < ev) alpha = ev;
	}

	return alpha;
}
