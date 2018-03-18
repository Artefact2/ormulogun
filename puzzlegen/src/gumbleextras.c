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

void cche_play_null_move(cch_board_t* b, cch_move_t* m, cch_undo_move_state_t* u) {
	/* XXX: this somewhat makes a lot of assumptions about gumble internals */
	/* XXX: finding an empty square like that is inefficient */
	for(unsigned char i = 0; i < 64; ++i) {
		if(CCH_GET_SQUARE(b, i)) continue;
		m->start = m->end = i;
		m->promote = 0;
		cch_play_legal_move(b, m, u);
		return;
	}

	__builtin_unreachable();
	assert(0);
}

bool cche_moves_through_square(const cch_move_t* m, cch_square_t sq) {
	char x1 = CCH_FILE(m->end) - CCH_FILE(m->start);
	char y1 = CCH_RANK(m->end) - CCH_RANK(m->start);
	char x2 = CCH_FILE(sq) - CCH_FILE(m->start);
	char y2 = CCH_RANK(sq) - CCH_RANK(m->start);

	if(x1 * y2 != x2 * y1) return false; /* Wrong direction */
	if(x1 * x2 < 0 || y1 * y2 < 0) return false; /* Wrong orientation */
	if(x1 * x1 + y1 * y1 < x2 * x2 + y2 * y2) return false; /* Too short */
	return true;
}

/* XXX: untested */
unsigned char cche_defenders_of_square(cch_board_t* b, cch_square_t sq, cch_movelist_t ml) {
	cch_piece_t p = CCH_GET_SQUARE(b, sq);
	if(CCH_IS_ENEMY_PIECE(b, p)) {
		cch_move_t m;
		cch_undo_move_state_t u;
		unsigned char r;

		cche_play_null_move(b, &m, &u);
		r = cche_defenders_of_square(b, sq, ml);
		cch_undo_move(b, &m, &u);
		return r;
	}

	unsigned char stop, i;

	CCH_SET_SQUARE(b, sq, CCH_MAKE_ENEMY_PIECE(b, CCH_PAWN));
	stop = cch_generate_moves(b, &(ml[1]), CCH_LEGAL, 0, 64);
	CCH_SET_SQUARE(b, sq, p);

	for(i = 1; i <= stop; ++i) {
		if(ml[i].end != sq) continue;
		cch_move_t m;
		cch_undo_move_state_t u, un;
		unsigned char r;

		ml[0] = ml[i];
		cch_play_legal_move(b, ml, &u);
		cche_play_null_move(b, &m, &un);
		r = cche_defenders_of_square(b, sq, &(ml[1]));
		cch_undo_move(b, &m, &un);
		cch_undo_move(b, ml, &u);
		return r + 1;
	}

	return 0;
}

bool cche_could_take(cch_board_t* b, cch_square_t src, cch_square_t dest) {
	if(CCH_IS_ENEMY_PIECE(b, CCH_GET_SQUARE(b, src))) {
		cch_move_t m;
		cch_undo_move_state_t u;
		bool r;

		cche_play_null_move(b, &m, &u);
		r = cche_could_take(b, src, dest);
		cch_undo_move(b, &m, &u);
		return r;
	}

	cch_piece_t p = CCH_GET_SQUARE(b, dest);
	cch_movelist_t ml;
	unsigned char stop, i;

	CCH_SET_SQUARE(b, dest, CCH_MAKE_ENEMY_PIECE(b, CCH_PAWN));
	stop = cch_generate_moves(b, ml, CCH_LEGAL, src, src + 1);
	CCH_SET_SQUARE(b, dest, p);

	for(i = 0; i < stop; ++i) {
		if(ml[i].end == dest) return true;
	}

	return false;
}
