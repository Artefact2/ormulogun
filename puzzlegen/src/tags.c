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
	MAYBE_PRINT_TAG(p->tags.draw, "Draw");
	MAYBE_PRINT_TAG(p->tags.stalemate, "Draw (Stalemate)");
	MAYBE_PRINT_TAG(p->tags.threefold, "Draw (Threefold repetition)");
	MAYBE_PRINT_TAG(p->tags.perpetual, "Draw (Perpetual check)");
	MAYBE_PRINT_TAG(p->tags.discovered_check, "Discovered check");
	MAYBE_PRINT_TAG(p->tags.double_check, "Double check");
	MAYBE_PRINT_TAG(p->tags.promotion, "Promotion");
	MAYBE_PRINT_TAG(p->tags.underpromotion, "Underpromotion");
	MAYBE_PRINT_TAG(p->tags.pin_absolute, "Pin");
	MAYBE_PRINT_TAG(p->tags.pin_absolute, "Pin (Absolute)");
	MAYBE_PRINT_TAG(p->tags.fork, "Fork");

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

static void tags_mate_threat(const uci_engine_context_t* ctx, puzzle_t* p, cch_board_t* b) {
	if(p->tags.mate_threat) return;

	uci_eval_t ev[5];
	unsigned char stop = uci_eval(ctx, "depth 1", b, ev, 5);
	if(stop == 0) return;

	if(ev[stop - 1].type == SCORE_MATE && ev[stop - 1].score == -1) {
		p->tags.mate_threat = true;
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

static void tags_pin_absolute(puzzle_t* p, cch_board_t* b, const cch_move_t* last) {
	if(p->tags.pin_absolute) return;

	cch_movelist_t ml;
	unsigned char i, stop = cch_generate_moves(b, ml, CCH_PSEUDO_LEGAL, 0, 64);

	for(i = 0; i < stop; ++i) {
		if(!CCH_GET_SQUARE(b, ml[i].end)) continue;
		if(ml[i].start == CCH_OWN_KING(b)) continue;
		/* XXX: not all pins involve being blocked from capturing the
		 * piece that just moved */
		if(ml[i].end != last->end) continue;
		if(cch_is_pseudo_legal_move_legal(b, &(ml[i]))) continue;
		p->tags.pin_absolute = true;
		break;
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

static void tags_step(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b, const uci_engine_context_t* ctx, unsigned char depth) {
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
		cch_play_legal_move(b, &(st->move), &um);

		if(CCH_IS_OWN_KING_CHECKED(b)) {
			tags_discovered_double_check(p, b, &(st->move));
		} else {
			tags_pin_absolute(p, b, &(st->move));
		}
		tags_mate_threat(ctx, p, b);
		tags_fork(p, st, b);

		if(st->reply.start == 255) {
			cch_undo_move(b, &(st->move), &um);
			return;
		}

		cch_play_legal_move(b, &(st->reply), &ur);
	}

	tags_promotion(p, st);

	for(i = 0; i < st->nextlen; ++i) {
		tags_step(p, &(st->next[i]), b, ctx, depth + 1);
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

void tags_puzzle(puzzle_t* p, cch_board_t* b, const uci_engine_context_t* ctx) {
	p->num_variations = 0;
	p->max_depth = 0;
	p->end_material_min = 255;
	p->end_material_max = 0;
	p->end_material_diff_min = 127;
	p->end_material_diff_max = -127;

	tags_step(p, &(p->root), b, ctx, 0);
}
