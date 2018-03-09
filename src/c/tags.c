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

#define DEBUG_LAN(str, move) do {						\
		char lan[SAFE_ALG_LENGTH];						\
		cch_format_lan_move((move), lan, SAFE_ALG_LENGTH);	\
		fprintf(stderr, "%s: %s\n", (str), lan);			\
	} while(0)


#define MAYBE_PRINT_TAG(f, n) do {				\
		if(f) {									\
			printf(",\"%s\"", (n));				\
		}										\
	} while(0)

void tags_print(const puzzle_t* p) {
	putchar('[');

	if(p->min_depth == p->max_depth) {
		printf("\"Depth %d\"", p->min_depth);
	} else {
		printf("\"Min depth %d\",\"Max depth %d\"", p->min_depth, p->max_depth);
	}

	MAYBE_PRINT_TAG(p->num_variations == 1, "Linear");
	MAYBE_PRINT_TAG(p->tags.checkmate, "Checkmate");
	MAYBE_PRINT_TAG(p->tags.winning_position, "Winning position");
	MAYBE_PRINT_TAG(p->tags.drawing_position, "Drawing position");
	MAYBE_PRINT_TAG(p->tags.draw, "Draw");
	MAYBE_PRINT_TAG(p->tags.stalemate, "Draw (Stalemate)");
	MAYBE_PRINT_TAG(p->tags.threefold, "Draw (Threefold repetition)");
	MAYBE_PRINT_TAG(p->tags.perpetual, "Draw (Perpetual check)");
	MAYBE_PRINT_TAG(p->tags.discovered_check, "Discovered check");
	MAYBE_PRINT_TAG(p->tags.double_check, "Double check");
	MAYBE_PRINT_TAG(p->tags.promotion, "Promotion");
	MAYBE_PRINT_TAG(p->tags.underpromotion, "Underpromotion");
	MAYBE_PRINT_TAG(p->tags.pin, "Pin");
	MAYBE_PRINT_TAG(p->tags.fork, "Fork");
	MAYBE_PRINT_TAG(p->tags.skewer, "Skewer");
	MAYBE_PRINT_TAG(p->tags.endgame, "Endgame");
	MAYBE_PRINT_TAG(p->tags.capturing_defender, "Capturing defender");

	if(!p->tags.draw && !p->tags.checkmate) {
		MAYBE_PRINT_TAG(p->tags.mate_threat, "Checkmate threat");
		MAYBE_PRINT_TAG(!p->tags.promotion && p->end_material_diff_min > p->start_material_diff, "Material gain");
		MAYBE_PRINT_TAG(p->end_material_diff_max == p->start_material_diff
						&& p->end_material_diff_min == p->start_material_diff
						&& p->end_material_max < p->start_material, "Trade");
		MAYBE_PRINT_TAG(p->end_material_diff_max == p->start_material_diff
						&& p->end_material_diff_min == p->start_material_diff
						&& p->end_material_max == p->start_material
						&& p->end_material_min == p->start_material, "Quiet");
	}

	putchar(']');
}

static void tags_mate_threat(puzzle_t* p, cch_board_t* b) {
	if(p->tags.mate_threat) return;

	cch_movelist_t ml, ml2, ml3;
	cch_undo_move_state_t u, u2;
	unsigned char stop, stop2;
	unsigned char i, j;

	stop = cch_generate_moves(b, ml, CCH_LEGAL, 0, 64); /* Opponent turn */
	for(i = 0; i < stop && !p->tags.mate_threat; ++i) {
		cch_play_legal_move(b, &(ml[i]), &u);
		stop2 = cch_generate_moves(b, ml2, CCH_LEGAL, 0, 64); /* Our turn */
		for(j = 0; j < stop2 && !p->tags.mate_threat; ++j) {
			cch_play_legal_move(b, &(ml2[j]), &u2);
			p->tags.mate_threat = CCH_IS_OWN_KING_CHECKED(b) && cch_generate_moves(b, ml3, CCH_LEGAL, 0, 64) == 0;
			cch_undo_move(b, &(ml2[j]), &u2);
		}
		cch_undo_move(b, &(ml[i]), &u);
	}
}

static void tags_discovered_double_check(puzzle_t* p, cch_board_t* b, const cch_move_t* last) {
	if(p->tags.discovered_check && p->tags.double_check) return;

	/* XXX: pure piece 7 is a dummy piece */
	cch_piece_t prev = CCH_GET_SQUARE(b, last->end), dummy = CCH_MAKE_ENEMY_PIECE(b, 7);
	CCH_SET_SQUARE(b, last->end, dummy);
	bool check = CCH_IS_OWN_KING_CHECKED(b);
	CCH_SET_SQUARE(b, last->end, prev);

	if(!check) return;

	cch_board_t copy = *b;
	unsigned char checkers = 0;
	unsigned char i;

	/* XXX: this is sub-obtimal, have cch_is_square_checked() return the checking square? */

	/* Replace all enemy pieces with dummies */
	for(i = 0; i < 64; ++i) {
		if(CCH_IS_ENEMY_PIECE(&copy, CCH_GET_SQUARE(&copy, i))) {
			CCH_SET_SQUARE(&copy, i, dummy);
		}
	}

	/* Check one by one which enemy piece causes check */
	for(i = 0; i < 64 && checkers < 2; ++i) {
		CCH_SET_SQUARE(&copy, i, CCH_GET_SQUARE(b, i));
		if(CCH_IS_OWN_KING_CHECKED(&copy)) {
			++checkers;
		}
		CCH_SET_SQUARE(&copy, i, dummy);
	}

	if(checkers > 1) p->tags.double_check = true;
	else p->tags.discovered_check = true;
}

static void tags_pin(puzzle_t* p, cch_board_t* b, const cch_move_t* last) {
	if(p->tags.pin) return;

	/* If you take the piece I just moved (pseudo-legal move), I get
	 * to capture your king or gain material by taking another piece
	 * by moving through the square you just freed */

	char ev;
	cch_movelist_t ml, ml2;
	cch_undo_move_state_t u, u2;
	unsigned char i, j, stop = cch_generate_moves(b, ml, CCH_PSEUDO_LEGAL, 0, 64), stop2;

	eval_material(b, true, 0, &ev);

	for(i = 0; i < stop && !p->tags.pin; ++i) {
		if(ml[i].end != last->end) continue; /* Not a take back */
		cch_play_legal_move(b, &(ml[i]), &u);
		stop2 = cch_generate_moves(b, ml2, CCH_LEGAL, 0, 64);
		for(j = 0; j < stop2 && !p->tags.pin; ++j) {
			if(ml2[j].end == last->end) continue; /* Not taking another piece */
			if(!CCH_GET_SQUARE(b, ml2[j].end)) continue; /* Not taking anything at all */
			char x1 = CCH_FILE(ml2[j].end) - CCH_FILE(ml2[j].start);
			char y1 = CCH_RANK(ml2[j].end) - CCH_RANK(ml2[j].start);
			char x2 = CCH_FILE(ml[i].start) - CCH_FILE(ml2[j].start);
			char y2 = CCH_RANK(ml[i].start) - CCH_RANK(ml2[j].start);
			if(x1 * y2 != x2 * y1) continue; /* Not going through the newly freed square (wrong direction) */
			if(x1 * x2 < 0 || y1 * y2 < 0) continue; /* Not going through the newly freed square (wrong orientation) */
			if(x1 * x1 + y1 * y1 < x2 * x2 + y2 * y2) continue; /* Not going far enough */

			cch_play_legal_move(b, &(ml2[j]), &u2);
			p->tags.pin = -eval_quiet_material(b, -127, 127) > ev;
			cch_undo_move(b, &(ml2[j]), &u2);
		}
		cch_undo_move(b, &(ml[i]), &u);
	}
}

static void tags_promotion(puzzle_t* p, const puzzle_step_t* st) {
	if(p->tags.promotion && p->tags.underpromotion) return;

	bool promote = false;
	bool queen_promote = false;
	unsigned char i;

	for(i = 0; i < st->nextlen; ++i) {
		if(st->next[i].move.promote) {
			promote = true;

			if(st->next[i].move.promote == CCH_QUEEN) {
				queen_promote = true;
				break;
			}
		}
	}

	if(promote) {
		p->tags.promotion = true;
		if(!queen_promote) {
			p->tags.underpromotion = true;
		}
	}
}

static unsigned char count_winning_exchanges(cch_board_t* b, char ev, unsigned char start, unsigned char stop, cch_move_t* out) {
	cch_movelist_t ml;
	cch_undo_move_state_t undo;
	unsigned char attacks = 0, nmoves = cch_generate_moves(b, ml, CCH_LEGAL, start, stop);

	for(unsigned char i = 0; i < nmoves; ++i) {
		if(!CCH_GET_SQUARE(b, ml[i].end)) continue;
		cch_play_legal_move(b, &(ml[i]), &undo);
		if(-eval_quiet_material(b, -127, 127) > ev) {
			out[attacks] = ml[i];
			++attacks;
		}
		cch_undo_move(b, &(ml[i]), &undo);
	}

	return attacks;
}

static void tags_fork(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b) {
	if(p->tags.fork) return;
	if(st->nextlen == 0) return;

	char ev;
	unsigned char attacks;
	cch_move_t moves[8]; /* XXX */

	eval_material(b, true, 0, &ev);

	/* If opponent does nothing, how many exchanges can we win with
	 * the piece that we just moved? */
	b->side = !b->side;
	attacks = count_winning_exchanges(b, ev, st->move.end, st->move.end + 1, moves);
	b->side = !b->side;

	/* Can we win more than 2 exchanges? */
	if(attacks < 2) return;

	/* Is the reply to move one of the attacked pieces ? */
	bool found = false;
	unsigned char i;
	for(i = 0; i < attacks; ++i) {
		if(moves[i].end == st->reply.start) {
			found = true;
			break;
		}
	}
	if(!found) return;

	/* Now that the opponent's reply has been played, can we still win
	 * one exchange with the same piece? */
	cch_undo_move_state_t ur;
	unsigned char afterattacks;
	cch_move_t aftermoves[8]; /* XXX */

	cch_play_legal_move(b, &(st->reply), &ur);
	afterattacks = count_winning_exchanges(b, ev, st->move.end, st->move.end + 1, aftermoves);
	cch_undo_move(b, &(st->reply), &ur);
	if(afterattacks == 0 || afterattacks == attacks) return;

	/* Is the follow up taking on one of these? */
	for(i = 0; i < st->nextlen; ++i) {
		if(!cch_is_move_in_list(&(st->next[i].move), aftermoves, afterattacks)) {
			return;
		}
	}

	p->tags.fork = true;
}

static void tags_skewer(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b) {
	if(p->tags.skewer) return;
	if(st->nextlen == 0) return;

	/* Only sliders can do skewers */
	cch_pure_piece_t pp = CCH_PURE_PIECE(CCH_GET_SQUARE(b, st->move.end));
	if(pp != CCH_QUEEN && pp != CCH_ROOK && pp != CCH_BISHOP) return;

	char ev;
	unsigned char attacks;
	cch_move_t moves[8];

	eval_material(b, true, 0, &ev);

	b->side = !b->side;
	attacks = count_winning_exchanges(b, ev, st->move.end, st->move.end + 1, moves);
	b->side = !b->side;

	if(attacks == 0) return;

	/* Is the reply moving one of the attacked pieces ? */
	unsigned char i;
	for(i = 0; i < attacks; ++i) {
		if(moves[i].end == st->reply.start) {
			break;
		}
	}
	if(i == attacks) return;

	char x1 = CCH_FILE(moves[i].end) - CCH_FILE(moves[i].start);
	char y1 = CCH_RANK(moves[i].end) - CCH_RANK(moves[i].start);
	for(unsigned char j = 0; j < st->nextlen; ++j) {
		/* Is the follow up taking the piece behind the piece that just moved ? */
		if(st->next[j].move.start != st->move.end) continue;
		if(!CCH_GET_SQUARE(b, st->next[j].move.end)) continue;
		char x2 = CCH_FILE(st->next[j].move.end) - CCH_FILE(st->next[j].move.start);
		char y2 = CCH_RANK(st->next[j].move.end) - CCH_RANK(st->next[j].move.start);

		/* Check direction */
		if(x1 * y2 != y1 * x2) continue;

		/* Check orientation */
		if(x1 * x2 < 0 || y1 * y2 < 0) continue;

		p->tags.skewer = true;
		return;
	}
}

static void tags_capturing_defender(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b) {
	if(p->tags.capturing_defender) return;
	if(st->nextlen == 0) return;

	/* I take piece A, you take back, I take piece B that is no longer
	 * defended by A and win material */

	/* Not a capture */
	if(!CCH_GET_SQUARE(b, st->move.end)) return;
	/* No take back */
	if(st->reply.end != st->move.end) return;

	cch_undo_move_state_t um, ur;
	cch_movelist_t ml;
	unsigned char stop;
	cch_piece_t q;
	char ev;

	eval_material(b, false, 0, &ev);
	cch_play_legal_move(b, &(st->move), &um);
	cch_play_legal_move(b, &(st->reply), &ur);

	for(unsigned char i = 0; i < st->nextlen; ++i) {
		if(!CCH_GET_SQUARE(b, st->next[i].move.end)) continue;
		if(st->next[i].move.end == st->move.end) continue;

		/* See if the previously taken piece could defend the taken
		 * piece */
		cch_undo_move(b, &(st->reply), &ur);
		cch_undo_move(b, &(st->move), &um);
		q = CCH_GET_SQUARE(b, st->next[i].move.end);
		b->side = !b->side;
		CCH_SET_SQUARE(b, st->next[i].move.end, CCH_MAKE_ENEMY_PIECE(b, 7)); /* XXX */
		stop = cch_generate_moves(b, ml, CCH_LEGAL, st->move.end, st->move.end + 1);
		b->side = !b->side;
		CCH_SET_SQUARE(b, st->next[i].move.end, q);
		cch_play_legal_move(b, &(st->move), &um);
		cch_play_legal_move(b, &(st->reply), &ur);

		unsigned char j;
		for(j = 0; j < stop; ++j) {
			if(ml[j].end == st->next[i].move.end) break;
		}
		if(j < stop) {
			if(eval_quiet_material(b, -127, 127) > ev) {
				/* Previous piece was a defender, and now we gain material */
				p->tags.capturing_defender = true;
				break;
			}
		}
	}

	cch_undo_move(b, &(st->reply), &ur);
	cch_undo_move(b, &(st->move), &um);
}

static void tags_endgame(puzzle_t* p, const cch_board_t* b) {
	if(p->tags.endgame) return;
	/* The definition of "endgame" is quite ambiguous and
	 * controversial among some players. Here I consider an endgame
	 * any position with 5 or less chessmen on the board, that is,
	 * when reasonably-sized endgame tables can be used. */
	unsigned char pieces = 0;
	for(unsigned char i = 0; i < 64; ++i) {
		if(CCH_GET_SQUARE(b, i)) {
			++pieces;
			if(pieces > 5) break;
		}
	}
	p->tags.endgame = pieces <= 5;
}

static void tags_step(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b, unsigned char depth) {
	cch_undo_move_state_t um, ur;
	unsigned char i;

	if(st->nextlen == 0) {
		++p->num_variations;
		if(depth < p->min_depth) {
			p->min_depth = depth;
		}
		if(depth > p->max_depth) {
			p->max_depth = depth;
		}
	}

	if(depth > 0) {
		if(st->reply.start == 255) return;

		tags_capturing_defender(p, st, b);

		cch_play_legal_move(b, &(st->move), &um);

		if(CCH_IS_OWN_KING_CHECKED(b)) {
			tags_discovered_double_check(p, b, &(st->move));
		}

		tags_mate_threat(p, b);
		tags_fork(p, st, b);
		tags_skewer(p, st, b);
		tags_pin(p, b, &(st->move));

		cch_play_legal_move(b, &(st->reply), &ur);
	}

	tags_promotion(p, st);

	for(i = 0; i < st->nextlen; ++i) {
		tags_step(p, &(st->next[i]), b, depth + 1);
	}

	if(st->nextlen == 0) {
		unsigned char total;
		char diff;
		eval_material(b, false, &total, &diff);
		if(total > p->end_material_max) p->end_material_max = total;
		if(total < p->end_material_min) p->end_material_min = total;
		if(diff > p->end_material_diff_max) p->end_material_diff_max = diff;
		if(diff < p->end_material_diff_min) p->end_material_diff_min = diff;
	}

	if(depth > 0) {
		cch_undo_move(b, &(st->reply), &ur);
		cch_undo_move(b, &(st->move), &um);
	}
}

void tags_puzzle(puzzle_t* p, cch_board_t* b) {
	p->num_variations = 0;
	p->max_depth = 0;
	p->end_material_min = 255;
	p->end_material_max = 0;
	p->end_material_diff_min = 127;
	p->end_material_diff_max = -127;

	tags_endgame(p, b);
	tags_step(p, &(p->root), b, 0);
}
