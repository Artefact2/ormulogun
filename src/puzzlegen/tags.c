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
#include <moves.h>

#define DEBUG_LAN(str, move) do {						\
		char lan[SAFE_ALG_LENGTH];						\
		cch_format_lan_move((move), lan, SAFE_ALG_LENGTH);	\
		fprintf(stderr, "%s: %s\n", (str), lan);			\
	} while(0)

#define PRINT_TAG(n) do {						\
		printf(",\"%s\"", (n));					\
	} while(0)

#define MAYBE_PRINT_TAG(f, n) do {				\
		if(f) {									\
			PRINT_TAG(n);						\
		}										\
	} while(0)

#define PRINT_PIECE_TAG(flags, fmt) do {								\
		if((flags >> (CCH_PAWN - 1)) & 1) printf(",\"" fmt "\"", "Pawn"); \
		if((flags >> (CCH_BISHOP - 1)) & 1) printf(",\"" fmt "\"", "Bishop"); \
		if((flags >> (CCH_KNIGHT - 1)) & 1) printf(",\"" fmt "\"", "Knight"); \
		if((flags >> (CCH_ROOK - 1)) & 1) printf(",\"" fmt "\"", "Rook"); \
		if((flags >> (CCH_QUEEN - 1)) & 1) printf(",\"" fmt "\"", "Queen"); \
		if((flags >> (CCH_KING - 1)) & 1) printf(",\"" fmt "\"", "King"); \
	} while(0)

#define SQUARE_NEIGHBORS(n, sq) do {			\
		n[0] = CCH_NORTH(sq);					\
		n[1] = CCH_SOUTH(sq);					\
		n[2] = CCH_WEST(sq);					\
		n[3] = CCH_EAST(sq);					\
		n[4] = CCH_NORTHEAST(sq);				\
		n[5] = CCH_NORTHWEST(sq);				\
		n[6] = CCH_SOUTHEAST(sq);				\
		n[7] = CCH_SOUTHWEST(sq);				\
	} while(0)



void tags_print(const puzzle_t* p) {
	putchar('[');

	if(p->min_depth == p->max_depth) {
		if(p->num_variations == 1) {
			printf("\"Linear\",\"Linear (Depth %d)\"", p->min_depth);
		} else {
			printf("\"Nonlinear\",\"Nonlinear (Depth %d)\"", p->min_depth);
		}
	} else {
		assert(p->num_variations >= 2);
		printf("\"Nonlinear\",\"Nonlinear (Depth %d-%d)\"", p->min_depth, p->max_depth);
	}

	MAYBE_PRINT_TAG(p->tags.endgame, "Endgame");
	MAYBE_PRINT_TAG(p->tags.endgame_q, "Endgame (Queen ending)");
	MAYBE_PRINT_TAG(p->tags.endgame_r, "Endgame (Rook ending)");
	MAYBE_PRINT_TAG(p->tags.endgame_b, "Endgame (Bishop ending)");
	MAYBE_PRINT_TAG(p->tags.endgame_n, "Endgame (Knight ending)");
	MAYBE_PRINT_TAG(p->tags.endgame_p, "Endgame (Pawn ending)");
	MAYBE_PRINT_TAG(p->tags.endgame_m, "Endgame (Minor piece ending)");
	MAYBE_PRINT_TAG(p->tags.endgame_M, "Endgame (Major piece ending)");
	MAYBE_PRINT_TAG(p->tags.endgame_Mm, "Endgame (Mixed piece ending)");

	MAYBE_PRINT_TAG(p->tags.checkmate, "Checkmate");
	PRINT_PIECE_TAG(p->tags.checkmate_piece, "Checkmate (%s)");

	if(p->tags.checkmate_smothered || p->tags.checkmate_suffocation
	|| p->tags.checkmate_back_rank || p->tags.checkmate_bodens
	|| p->tags.checkmate_grecos || p->tags.checkmate_anastasias
	|| p->tags.checkmate_arabian || p->tags.checkmate_corner
	|| p->tags.checkmate_dovetail || p->tags.checkmate_swallows_tail
	|| p->tags.checkmate_epaulette) {
		PRINT_TAG("Checkmate pattern");
		MAYBE_PRINT_TAG(p->tags.checkmate_smothered, "Checkmate pattern (Smothered mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_suffocation, "Checkmate pattern (Suffocation mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_back_rank, "Checkmate pattern (Back-rank mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_bodens, "Checkmate pattern (Boden's mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_grecos, "Checkmate pattern (Greco's mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_anastasias, "Checkmate pattern (Anastasia's mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_arabian, "Checkmate pattern (Arabian mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_corner, "Checkmate pattern (Corner mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_dovetail, "Checkmate pattern (Dovetail/Cozio's mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_swallows_tail, "Checkmate pattern (Swallow's tail/Guéridon mate)");
		MAYBE_PRINT_TAG(p->tags.checkmate_epaulette, "Checkmate pattern (Epaulette mate)");
	}

	MAYBE_PRINT_TAG(p->tags.winning_position, "Winning position");
	MAYBE_PRINT_TAG(p->tags.drawing_position, "Drawing position");

	MAYBE_PRINT_TAG(p->tags.stalemate || p->tags.threefold || p->tags.perpetual, "Draw");
	MAYBE_PRINT_TAG(p->tags.stalemate, "Draw (Stalemate)");
	MAYBE_PRINT_TAG(p->tags.threefold, "Draw (Threefold repetition)");
	MAYBE_PRINT_TAG(p->tags.perpetual, "Draw (Perpetual check)");

	MAYBE_PRINT_TAG(p->tags.discovered_attack, "Discovered attack");
	MAYBE_PRINT_TAG(p->tags.discovered_check, "Discovered check");
	MAYBE_PRINT_TAG(p->tags.double_check, "Double check");

	MAYBE_PRINT_TAG(p->tags.promotion, "Promotion");
	MAYBE_PRINT_TAG(p->tags.underpromotion, "Underpromotion");

	MAYBE_PRINT_TAG(p->tags.pin, "Pin");
	MAYBE_PRINT_TAG(p->tags.fork, "Fork");
	MAYBE_PRINT_TAG(p->tags.skewer, "Skewer");
	MAYBE_PRINT_TAG(p->tags.undermining, "Undermining");
	MAYBE_PRINT_TAG(p->tags.trapped_piece, "Trapped piece");
	MAYBE_PRINT_TAG(p->tags.overloaded_piece, "Overloaded piece");
	MAYBE_PRINT_TAG(p->tags.deflection, "Deflection");

	if(!p->tags.checkmate && !p->tags.threefold && !p->tags.stalemate && !p->tags.perpetual && !p->tags.winning_position && !p->tags.drawing_position) {
		MAYBE_PRINT_TAG(p->tags.mate_threat, "Checkmate threat");

		if(p->end_material_diff_max > p->start_material_diff) {
			MAYBE_PRINT_TAG(!p->tags.promotion, "Material gain");
		} else if(p->end_material_diff_max < p->start_material_diff) {
			PRINT_TAG("Material loss");
		} else {
			if(p->end_material_min == p->start_material) {
				PRINT_TAG("Quiet");
			}  else {
				PRINT_TAG("Trade");
			}
		}
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
			if(!cche_moves_through_square(&(ml2[j]), ml[i].start)) continue; /* Not going through the newly freed square */

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
	cch_move_t nullmove;
	cch_undo_move_state_t um;

	eval_material(b, true, 0, &ev);

	/* If opponent does nothing, how many exchanges can we win with
	 * the piece that we just moved? */
	cche_play_null_move(b, &nullmove, &um);
	attacks = count_winning_exchanges(b, ev, st->move.end, st->move.end + 1, moves);
	cch_undo_move(b, &nullmove, &um);

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
	cch_move_t nullmove;
	cch_undo_move_state_t unull;

	eval_material(b, true, 0, &ev);

	cche_play_null_move(b, &nullmove, &unull);
	attacks = count_winning_exchanges(b, ev, st->move.end, st->move.end + 1, moves);
	cch_undo_move(b, &nullmove, &unull);

	if(attacks == 0) return;

	/* Is the reply moving one of the attacked pieces ? */
	unsigned char i;
	for(i = 0; i < attacks; ++i) {
		if(moves[i].end == st->reply.start) {
			break;
		}
	}
	if(i == attacks) return;

	for(unsigned char j = 0; j < st->nextlen; ++j) {
		/* Is the follow up taking the piece behind the piece that just moved ? */
		if(st->next[j].move.start != st->move.end) continue;
		if(!CCH_GET_SQUARE(b, st->next[j].move.end)) continue;
		if(!cche_moves_through_square(&(st->next[j].move), st->reply.start)) continue;

		p->tags.skewer = true;
		return;
	}
}

static void tags_undermining(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b) {
	if(p->tags.undermining) return;
	if(st->nextlen == 0) return;

	/* I take piece A, you take back, I take piece B that is no longer
	 * defended by A and win material */

	/* Not a capture */
	if(!CCH_GET_SQUARE(b, st->move.end)) return;
	/* No take back */
	if(st->reply.end != st->move.end) return;

	cch_undo_move_state_t um, ur;
	char ev, qev;

	eval_material(b, false, 0, &ev);
	cch_play_legal_move(b, &(st->move), &um);
	cch_play_legal_move(b, &(st->reply), &ur);
	qev = eval_quiet_material(b, -127, 127);
	cch_undo_move(b, &(st->reply), &ur);
	cch_undo_move(b, &(st->move), &um);
	if(qev <= ev) return; /* Not gaining material */

	for(unsigned char i = 0; i < st->nextlen; ++i) {
		if(!CCH_GET_SQUARE(b, st->next[i].move.end)) continue;
		if(st->next[i].move.end == st->move.start || st->next[i].move.end == st->move.end || st->next[i].move.end == st->reply.start) continue;

		if(cche_could_take(b, st->move.end, st->next[i].move.end, CCH_LEGAL)) {
			p->tags.undermining = true;
			return;
		}
	}
}

static void tags_deflection(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b) {
	if(p->tags.deflection) return;
	if(st->nextlen == 0) return;

	/* I move piece A, attacking piece B. You move piece B to a square
	 * where it can no longer defend piece C. I capture piece C and
	 * gain material. */

	if(!cche_could_take(b, st->move.end, st->reply.start, CCH_LEGAL)) return;

	char ev, qev;
	eval_material(b, false, 0, &ev);

	for(unsigned char i = 0; i < st->nextlen; ++i) {
		if(!CCH_GET_SQUARE(b, st->next[i].move.end)) continue;
		if(st->next[i].move.end == st->reply.start || st->next[i].move.end == st->reply.end) continue;

		cch_undo_move_state_t u;
		bool can_defend;

		cch_play_legal_move(b, &st->next[i].move, &u);
		can_defend = cche_could_take(b, st->reply.start, st->next[i].move.end, CCH_LEGAL);
		cch_undo_move(b, &st->next[i].move, &u);

		if(!can_defend) continue;

		cch_play_legal_move(b, &st->reply, &u);
		can_defend = cche_could_take(b, st->reply.end, st->next[i].move.end, CCH_LEGAL);
		qev = eval_quiet_material(b, -127, 127);
		cch_undo_move(b, &st->reply, &u);

		if(can_defend) continue;
		if(qev <= ev) continue;

		p->tags.deflection = true;
		return;
	}
}

static void tags_trapped_piece(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b) {
	if(p->tags.trapped_piece) return;
	if(st->nextlen == 0) return;

	/* Did our last move threaten to capture a piece? If yes, does
	 * every possible move for that piece result in a loss of material
	 * ? */

	cch_movelist_t ml, ml2, ml3;
	unsigned char stop, stop2, stop3;
	unsigned char i, j, k;
	char ev;
	cch_undo_move_state_t u;

	cche_play_null_move(b, ml2, &u);
	eval_material(b, false, 0, &ev);
	stop = cch_generate_moves(b, ml, CCH_LEGAL, st->move.end, st->move.end + 1);
	cch_undo_move(b, ml2, &u);

	for(i = 0; i < stop && !p->tags.trapped_piece; ++i) {
		if(!CCH_GET_SQUARE(b, ml[i].end)) continue;
		stop2 = cch_generate_moves(b, ml2, CCH_LEGAL, ml[i].end, ml[i].end + 1);

		for(j = 0; j < stop2; ++j) {
			cch_play_legal_move(b, &(ml2[j]), &u);
			stop3 = cch_generate_moves(b, ml3, CCH_LEGAL, 0, 64);
			for(k = 0; k < stop3; ++k) {
				if(ml3[k].end != ml2[j].end) continue; /* Not a take back */
				if(eval_quiet_material(b, -127, 127) > ev) {
					/* We can take back the piece and gain material */
					break;
				}
			}
			if(k == stop3) {
				/* We didn't break from the loop... That means the
				 * piece has a safe flight square */
				stop2 = 0;
			}
			cch_undo_move(b, &(ml2[j]), &u);
		}

		if(stop2 == 0 || j < stop2) continue;

		/* Do we end up taking the piece? */

		unsigned char fpos = st->reply.start == ml[i].end ? st->reply.end : ml[i].end;
		for(j = 0; j < st->nextlen; ++j) {
			if(st->next[j].move.end == fpos) {
				p->tags.trapped_piece = true;
				break;
			}
		}
	}
}

static void tags_discovered_attack(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b) {
	if(p->tags.discovered_attack && p->tags.discovered_check && p->tags.double_check) return;
	if(st->nextlen == 0) return;

	cch_movelist_t ml;
	unsigned char stop;
	cch_move_t nullmove;
	cch_undo_move_state_t u, unull;
	char ev;
	bool disco_check = false;
	unsigned char i;

	cche_play_null_move(b, &nullmove, &unull);
	eval_material(b, false, 0, &ev);
	stop = cch_generate_moves(b, ml, CCH_LEGAL, 0, 64);

	for(i = 0; i < stop; ++i) {
		if(ml[i].start == st->move.end) continue; /* Can't do a discovered attack WITH the piece that just moved! */
		if(!CCH_GET_SQUARE(b, ml[i].end)) continue; /* Not attacking anything */

		/* Are we going through the newly freed square? */
		if(ml[i].end == st->move.start || !cche_moves_through_square(&(ml[i]), st->move.start)) continue;

		if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, ml[i].end)) == CCH_KING) {
			disco_check = true; /* Discovered check! */
			continue;
		}

		/* Is the attack actually meaningful? */
		cch_play_legal_move(b, &(ml[i]), &u);
		if(-eval_quiet_material(b, -127, 127) > ev) {
			p->tags.discovered_attack = true;
		}
		cch_undo_move(b, &(ml[i]), &u);
	}

	if(disco_check) {
		/* Check for double check */
		for(i = 0; i < stop; ++i) {
			if(ml[i].start != st->move.end) continue;
			if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, ml[i].end)) == CCH_KING) {
				p->tags.double_check = true;
				break;
			}
		}
		if(i == stop) {
			p->tags.discovered_check = true;
		}
	}

	cch_undo_move(b, &nullmove, &unull);
}

static void tags_overloaded_piece(puzzle_t* p, const puzzle_step_t* st, cch_board_t* b) {
	if(p->tags.overloaded_piece) return;
	if(st->nextlen == 0) return;

	/* I move to a square A, you took back with piece P. Sadly piece P
	 * is now no longer defending piece Q which I can now take with
	 * one of my pieces and gain material. */

	if(st->reply.end != st->move.end) return;

	for(unsigned char i = 0; i < st->nextlen; ++i) {
		if(!CCH_GET_SQUARE(b, st->next[i].move.end)) return;
		if(st->next[i].move.end == st->reply.end) {
			/* Move/take/take back, not really a case of overloading */
			return;
		}
		if(!cche_could_take(b, st->reply.start, st->next[i].move.end, CCH_LEGAL)) {
			/* Piece that took back wasn't a defender of Q */
			return;
		}

		cch_undo_move_state_t u;
		bool can_still_defend;
		cch_play_legal_move(b, &st->reply, &u);
		can_still_defend = cche_could_take(b, st->reply.end, st->next[i].move.end, CCH_LEGAL);
		cch_undo_move(b, &st->reply, &u);

		if(can_still_defend) {
			/* Piece that took back can still defend Q */
			return;
		}
	}

	p->tags.overloaded_piece = true;
}

static void tags_endgame(puzzle_t* p, const cch_board_t* b) {
	/* Are there 5 or less chessmen on the board ? */
	unsigned char pieces, wpieces = 0, bpieces = 0, pawns = 0, rooks = 0, queens = 0, bishops = 0, knights = 0;
	cch_piece_t piece;
	for(unsigned char i = 0; i < 64; ++i) {
		piece = CCH_GET_SQUARE(b, i);
		if(!piece) continue;

		if(CCH_PURE_PIECE(piece) == CCH_KING) continue;
		if(CCH_PURE_PIECE(piece) == CCH_PAWN) {
			++pawns;
			continue;
		}

		switch(CCH_PURE_PIECE(piece)) {
		case CCH_QUEEN: ++queens; break;
		case CCH_ROOK: ++rooks; break;
		case CCH_BISHOP: ++bishops; break;
		case CCH_KNIGHT: ++knights; break;
		}

		if(CCH_IS_OWN_PIECE(b, piece)) ++wpieces;
		else ++bpieces;
	}

	pieces = wpieces + bpieces;
	if(wpieces > 1 || bpieces > 1) return;

	p->tags.endgame_q = queens  > 0 && pieces == queens;
	p->tags.endgame_r = rooks   > 0 && pieces == rooks;
	p->tags.endgame_b = bishops > 0 && pieces == bishops;
	p->tags.endgame_n = knights > 0 && pieces == knights;
	p->tags.endgame_p = pawns   > 0 && pieces == 0;

	p->tags.endgame_m = knights == 1 && bishops == 1;
	p->tags.endgame_M = queens == 1 && rooks == 1;
	p->tags.endgame_Mm = (knights + bishops == 1) && (rooks + queens == 1);

	p->tags.endgame = true;
}

static void tags_smothered_mate(puzzle_t* p, const puzzle_step_t* leaf, const cch_board_t* b) {
	/* King is surrounded by friendly pieces */

	cch_square_t k = CCH_OWN_KING(b);
	cch_square_t neighbors[8];
	SQUARE_NEIGHBORS(neighbors, k);

	for(unsigned char i = 0; i < 8; ++i) {
		if(CCH_IS_SQUARE_VALID(neighbors[i]) && !CCH_IS_OWN_PIECE(b, CCH_GET_SQUARE(b, neighbors[i]))) return;
	}

	p->tags.checkmate_smothered = true;
}

static void tags_suffocation_mate(puzzle_t* p, const puzzle_step_t* leaf, cch_board_t* b) {
	/* King is attacked by a knight, all escape squares are defended by a bishop */

	if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, leaf->move.end)) != CCH_KNIGHT) return;

	cch_square_t k = CCH_OWN_KING(b);
	if(!cche_could_take(b, leaf->move.end, k, CCH_PSEUDO_LEGAL)) return;

	cch_square_t neighbors[8];
	SQUARE_NEIGHBORS(neighbors, k);

	cch_square_t attacker = 255;
	cch_movelist_t ml;
	unsigned char stop;
	cch_move_t m;
	cch_undo_move_state_t u;

	m.start = k;
	m.promote = 0;

	for(unsigned char i = 0; i < 8; ++i) {
		cch_square_t sq = neighbors[i];
		if(!CCH_IS_SQUARE_VALID(sq)) continue;
		if(CCH_IS_ENEMY_PIECE(b, CCH_GET_SQUARE(b, sq))) return;
		if(CCH_GET_SQUARE(b, sq)) continue;

		m.end = sq;
		cch_play_legal_move(b, &m, &u);
		stop = cche_own_takers_of_square(b, sq, ml, CCH_PSEUDO_LEGAL);
		cch_undo_move(b, &m, &u);

		assert(stop >= 1); /* King has escape squares? How are we being checkmated?! */
		if(stop > 1) return; /* Square defended more than once */
		assert(ml[0].end == sq);
		if(attacker == 255) {
			attacker = ml[0].start;
		} else if(attacker != ml[0].start) {
			/* Different escape squares are protected by different pieces */
			return;
		}
	}

	if(attacker == 255 || CCH_PURE_PIECE(CCH_GET_SQUARE(b, attacker)) != CCH_BISHOP) return;

	p->tags.checkmate_suffocation = true;
}

static void tags_back_rank_mate(puzzle_t* p, const puzzle_step_t* leaf, const cch_board_t* b) {
	/* King is in the back rank, front rank is blocked by friendly
	 * pieces, queen or rook delivers checkmate */

	cch_pure_piece_t piece = CCH_PURE_PIECE(CCH_GET_SQUARE(b, leaf->move.end));
	if(piece != CCH_QUEEN && piece != CCH_ROOK) return;

	cch_square_t k = CCH_OWN_KING(b);
	cch_square_t front[3];
	switch(CCH_RANK(k)) {
	case 0:
		front[0] = CCH_NORTHEAST(k);
		front[1] = CCH_NORTH(k);
		front[2] = CCH_NORTHWEST(k);
		break;

	case 7:
		front[0] = CCH_SOUTHEAST(k);
		front[1] = CCH_SOUTH(k);
		front[2] = CCH_SOUTHWEST(k);
		break;

	default: return;
	}

	for(unsigned char i = 0; i < 3; ++i) {
		if(!CCH_IS_OWN_PIECE(b, CCH_GET_SQUARE(b, front[i]))) return;
	}

	p->tags.checkmate_back_rank = true;
}

static void tags_bodens_mate(puzzle_t* p, const puzzle_step_t* leaf, cch_board_t* b) {
	/* Two bishop deliver checkmate on criss-crossing diagonals. */

	if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, leaf->move.end)) != CCH_BISHOP) return;

	cch_square_t k = CCH_OWN_KING(b);
	cch_square_t n[8];
	SQUARE_NEIGHBORS(n, k);

	cch_movelist_t ml;
	unsigned char stop;
	cch_move_t m;
	cch_undo_move_state_t u;

	bool crossing_diagonals = false;
	char x1 = CCH_FILE(k) - CCH_FILE(leaf->move.end);
	char y1 = CCH_RANK(k) - CCH_RANK(leaf->move.end);
	char x2, y2;

	m.start = k;
	m.promote = 0;

	for(unsigned char i = 0; i < 8; ++i) {
		if(!CCH_IS_SQUARE_VALID(n[i])) continue;
		if(CCH_IS_ENEMY_PIECE(b, CCH_GET_SQUARE(b, n[i]))) return;
		if(CCH_GET_SQUARE(b, n[i])) continue;

		m.end = n[i];
		cch_play_legal_move(b, &m, &u);
		stop = cche_own_takers_of_square(b, n[i], ml, CCH_PSEUDO_LEGAL);
		cch_undo_move(b, &m, &u);

		assert(stop >= 1);
		if(stop > 1) return;
		if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, ml[0].start)) != CCH_BISHOP) return;

		x2 = CCH_FILE(ml[0].end) - CCH_FILE(ml[0].start);
		y2 = CCH_RANK(ml[0].end) - CCH_RANK(ml[0].start);
		if(x1 * y2 != x2 * y1) crossing_diagonals = true;
	}

	if(!crossing_diagonals) return;
	p->tags.checkmate_bodens = true;
}

static void tags_grecos_mate(puzzle_t* p, const puzzle_step_t* leaf, cch_board_t* b) {
	/* A queen or rook delivers checkmate on a cornered king, blocked
	 * diagonally by a friendly piece. The other flight square is
	 * covered by a bishop. */

	cch_pure_piece_t piece = CCH_PURE_PIECE(CCH_GET_SQUARE(b, leaf->move.end));
	if(piece != CCH_QUEEN && piece != CCH_ROOK) return;

	cch_square_t k = CCH_OWN_KING(b);
	/* Queen delivers checkmate diagonally- not a Greco's mate */
	if(CCH_RANK(k) != CCH_RANK(leaf->move.end) && CCH_FILE(k) != CCH_FILE(leaf->move.end)) return;

	bool same_rank = (CCH_RANK(k) == CCH_RANK(leaf->move.end));

	/* Discovered checkmate, probably not a Greco's mate */
	if(!cche_could_take(b, leaf->move.end, k, CCH_PSEUDO_LEGAL)) return;

	cch_square_t dsq, bsq;
	switch(k) {
	case SQ_A1:
		dsq = CCH_NORTHEAST(k);
		bsq = same_rank ? CCH_NORTH(k) : CCH_EAST(k);
		break;

	case SQ_A8:
		dsq = CCH_SOUTHEAST(k);
		bsq = same_rank ? CCH_SOUTH(k) : CCH_EAST(k);
		break;

	case SQ_H1:
		dsq = CCH_NORTHWEST(k);
		bsq = same_rank ? CCH_NORTH(k) : CCH_WEST(k);
		break;

	case SQ_H8:
		dsq = CCH_SOUTHWEST(k);
		bsq = same_rank ? CCH_SOUTH(k) : CCH_WEST(k);
		break;

	default: return; /* King is not in a corner */
	}

	if(!CCH_IS_OWN_PIECE(b, CCH_GET_SQUARE(b, dsq))) return;

	cch_movelist_t ml;
	unsigned char stop = cche_enemy_takers_of_square(b, bsq, ml, CCH_PSEUDO_LEGAL);
	if(stop != 1 || CCH_PURE_PIECE(CCH_GET_SQUARE(b, ml[0].start)) != CCH_BISHOP) return;

	p->tags.checkmate_grecos = true;
}

static void tags_anastasias_mate(puzzle_t* p, const puzzle_step_t* leaf, cch_board_t* b) {
	/* King is on the side of the board, with a friendly piece in
	 * front of it. A rook delivers checkmate while a knight attacks
	 * the two flight squares adjacent to the friendly piece. */

	if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, leaf->move.end)) != CCH_ROOK) return;

	cch_square_t k = CCH_OWN_KING(b);
	if(!cche_could_take(b, leaf->move.end, k, CCH_PSEUDO_LEGAL)) return;

	cch_square_t fsq[2] = { 255, 255 }; /* Friendly piece square */
	cch_square_t nsq[2] = { 255, 255 }; /* Knight square */
	cch_square_t esq[2][2] = { { 255, 255 }, { 255, 255 } }; /* Empty squares */

	switch(CCH_FILE(k)) {
	case 0:
		fsq[0] = CCH_EAST(k);
		nsq[0] = CCH_EAST(CCH_EAST(fsq[0]));
		esq[0][0] = CCH_NORTH(fsq[0]);
		esq[0][1] = CCH_SOUTH(fsq[0]);
		break;

	case 7:
		fsq[0] = CCH_WEST(k);
		nsq[0] = CCH_WEST(CCH_WEST(fsq[0]));
		esq[0][0] = CCH_NORTH(fsq[0]);
		esq[0][1] = CCH_SOUTH(fsq[0]);
		break;
	}

	switch(CCH_RANK(k)) {
	case 0:
		fsq[1] = CCH_NORTH(k);
		nsq[1] = CCH_NORTH(CCH_NORTH(fsq[1]));
		esq[1][0] = CCH_EAST(fsq[1]);
		esq[1][1] = CCH_WEST(fsq[1]);
		break;

	case 7:
		fsq[1] = CCH_SOUTH(k);
		nsq[1] = CCH_SOUTH(CCH_SOUTH(fsq[1]));
		esq[1][0] = CCH_EAST(fsq[1]);
		esq[1][1] = CCH_WEST(fsq[1]);
		break;
	}

	for(unsigned char i = 0; i < 2; ++i) {
		if(!CCH_IS_SQUARE_VALID(fsq[i])) continue;
		assert(CCH_IS_SQUARE_VALID(nsq[i]));
		assert(CCH_IS_SQUARE_VALID(esq[i][0]) || CCH_IS_SQUARE_VALID(esq[i][1]));

		if(!CCH_IS_OWN_PIECE(b, CCH_GET_SQUARE(b, fsq[i]))) continue;
		if(!CCH_IS_ENEMY_PIECE(b, CCH_GET_SQUARE(b, nsq[i]))) continue;
		if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, nsq[i])) != CCH_KNIGHT) continue;

		if(!CCH_IS_SQUARE_VALID(esq[i][0]) && !CCH_GET_SQUARE(b, esq[i][1])) continue;
		if(!CCH_IS_SQUARE_VALID(esq[i][1]) && !CCH_GET_SQUARE(b, esq[i][0])) continue;
		if(!CCH_GET_SQUARE(b, esq[i][0]) && !CCH_GET_SQUARE(b, esq[i][1])) continue;

		p->tags.checkmate_anastasias = true;
		return;
	}
}

static void tags_arabian_mate(puzzle_t* p, const puzzle_step_t* leaf, const cch_board_t* b) {
	/* King is mated in a corner by an adjacent rook. Rook and other
	 * flight square are covered by a knight. */

	cch_square_t k = CCH_OWN_KING(b);
	cch_square_t nsq, rsq[2];

	switch(k) {
	case SQ_A1:
		rsq[0] = CCH_NORTH(k);
		rsq[1] = CCH_EAST(k);
		nsq = CCH_NORTHEAST(CCH_NORTHEAST(k));
		break;

	case SQ_A8:
		rsq[0] = CCH_SOUTH(k);
		rsq[1] = CCH_EAST(k);
		nsq = CCH_SOUTHEAST(CCH_SOUTHEAST(k));
		break;

	case SQ_H1:
		rsq[0] = CCH_NORTH(k);
		rsq[1] = CCH_WEST(k);
		nsq = CCH_NORTHWEST(CCH_NORTHWEST(k));
		break;

	case SQ_H8:
		rsq[0] = CCH_SOUTH(k);
		rsq[1] = CCH_WEST(k);
		nsq = CCH_SOUTHWEST(CCH_SOUTHWEST(k));
		break;

	default: return;
	}

	assert(CCH_IS_SQUARE_VALID(nsq));
	assert(CCH_IS_SQUARE_VALID(rsq[0]));
	assert(CCH_IS_SQUARE_VALID(rsq[1]));

	if(!CCH_IS_ENEMY_PIECE(b, CCH_GET_SQUARE(b, nsq))) return;
	if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, nsq)) != CCH_KNIGHT) return;
	if(leaf->move.end != rsq[0] && leaf->move.end != rsq[1]) return;
	if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, leaf->move.end)) != CCH_ROOK) return;

	p->tags.checkmate_arabian = true;
}

static void tags_corner_mate(puzzle_t* p, const puzzle_step_t* leaf, cch_board_t* b) {
	/* King is mated by a knight in a corner of the board. A friendly
	 * piece blocks the file or rank, and the other escape file or
	 * rank is covered by a rook or a queen. */

	cch_square_t k = CCH_OWN_KING(b);
	cch_square_t nsq[2], fsq[2], esq[2][2];

	switch(k) {
	case SQ_A1:
		nsq[0] = CCH_NNE(k);
		fsq[0] = CCH_EAST(k);
		esq[0][0] = CCH_NORTH(k);
		esq[0][1] = CCH_NORTHEAST(k);
		nsq[1] = CCH_EEN(k);
		fsq[1] = CCH_NORTH(k);
		esq[1][0] = CCH_EAST(k);
		esq[1][1] = CCH_NORTHEAST(k);
		break;

	case SQ_A8:
		nsq[0] = CCH_SSE(k);
		fsq[0] = CCH_EAST(k);
		esq[0][0] = CCH_SOUTH(k);
		esq[0][1] = CCH_SOUTHEAST(k);
		nsq[1] = CCH_EES(k);
		fsq[1] = CCH_SOUTH(k);
		esq[1][0] = CCH_EAST(k);
		esq[1][1] = CCH_SOUTHEAST(k);
		break;

	case SQ_H1:
		nsq[0] = CCH_NNW(k);
		fsq[0] = CCH_WEST(k);
		esq[0][0] = CCH_NORTH(k);
		esq[0][1] = CCH_NORTHWEST(k);
		nsq[1] = CCH_WWN(k);
		fsq[1] = CCH_NORTH(k);
		esq[1][0] = CCH_WEST(k);
		esq[1][1] = CCH_NORTHWEST(k);
		break;

	case SQ_H8:
		nsq[0] = CCH_SSW(k);
		fsq[0] = CCH_WEST(k);
		esq[0][0] = CCH_SOUTH(k);
		esq[0][1] = CCH_SOUTHWEST(k);
		nsq[1] = CCH_WWS(k);
		fsq[1] = CCH_SOUTH(k);
		esq[1][0] = CCH_WEST(k);
		esq[1][1] = CCH_SOUTHWEST(k);
		break;

	default: return;
	}

	for(unsigned char i = 0; i < 2; ++i) {
		if(leaf->move.end != nsq[i]) continue;
		if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, nsq[i])) != CCH_KNIGHT) continue;
		if(!CCH_IS_OWN_PIECE(b, CCH_GET_SQUARE(b, fsq[i]))) continue;

		cch_movelist_t ml;
		unsigned char stop = cche_enemy_takers_of_square(b, esq[i][0], ml, CCH_PSEUDO_LEGAL);
		unsigned char stop2 = cche_enemy_takers_of_square(b, esq[i][1], &ml[stop], CCH_PSEUDO_LEGAL);
		if(stop == 0 || stop2 == 0) continue;
		for(unsigned char j = 0; j < stop; ++j) {
			for(unsigned char k = 0; k < stop2; ++k) {
				if(ml[j].start != ml[stop + k].start) continue;
				cch_pure_piece_t piece = CCH_PURE_PIECE(CCH_GET_SQUARE(b, ml[j].start));
				if(piece != CCH_ROOK && piece != CCH_QUEEN) continue;
				p->tags.checkmate_corner = true;
				return;
			}
		}
	}
}

static void tags_tail_mate(puzzle_t* p, const puzzle_step_t* leaf, const cch_board_t* b) {
	/* Dovetail mate: king is checkmated by a diagonally adjacent queen. The two
	 * escape squares (knight's distance from the queen) are covered
	 * by friendly pieces. */

	/* Swallow's tail mate: king is mated by an orthogonally adjacent
	 * queen. The two escape squares are blocked by friendly
	 * pieces. */

	if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, leaf->move.end)) != CCH_QUEEN) return;

	cch_square_t k = CCH_OWN_KING(b);
	cch_square_t dsq[2];
	bool dove;

	if(CCH_NORTHEAST(k) == leaf->move.end) {
		dsq[0] = CCH_SOUTH(k);
		dsq[1] = CCH_WEST(k);
		dove = true;
	} else if(CCH_NORTHWEST(k) == leaf->move.end) {
		dsq[0] = CCH_SOUTH(k);
		dsq[1] = CCH_EAST(k);
		dove = true;
	} else if(CCH_SOUTHEAST(k) == leaf->move.end) {
		dsq[0] = CCH_NORTH(k);
		dsq[1] = CCH_WEST(k);
		dove = true;
	} else if(CCH_SOUTHWEST(k) == leaf->move.end) {
		dsq[0] = CCH_NORTH(k);
		dsq[1] = CCH_EAST(k);
		dove = true;
	} else if(CCH_NORTH(k) == leaf->move.end) {
		dsq[0] = CCH_SOUTHEAST(k);
		dsq[1] = CCH_SOUTHWEST(k);
		dove = false;
	} else if(CCH_SOUTH(k) == leaf->move.end) {
		dsq[0] = CCH_NORTHEAST(k);
		dsq[1] = CCH_NORTHWEST(k);
		dove = false;
	} else if(CCH_EAST(k) == leaf->move.end) {
		dsq[0] = CCH_NORTHWEST(k);
		dsq[1] = CCH_SOUTHWEST(k);
		dove = false;
	} else if(CCH_WEST(k) == leaf->move.end) {
		dsq[0] = CCH_NORTHEAST(k);
		dsq[1] = CCH_SOUTHEAST(k);
		dove = false;
	} else return;

	if(!CCH_IS_SQUARE_VALID(dsq[0]) || !CCH_IS_SQUARE_VALID(dsq[1])) return;
	if(!CCH_IS_OWN_PIECE(b, CCH_GET_SQUARE(b, dsq[0])) || !CCH_IS_OWN_PIECE(b, CCH_GET_SQUARE(b, dsq[1]))) return;

	/* Using a pointer would be cleverer, but that's not possible with
	 * bitfields */
	if(dove) {
		p->tags.checkmate_dovetail = true;
	} else {
		p->tags.checkmate_swallows_tail = true;
	}
}

static void tags_epaulette_mate(puzzle_t* p, const puzzle_step_t* leaf, cch_board_t* b) {
	/* XXX: very ambiguous */
	/* Two parallel retreat squares are blocked by own pieces */

	if(CCH_PURE_PIECE(CCH_GET_SQUARE(b, leaf->move.end)) == CCH_KNIGHT) return;

	cch_square_t k = CCH_OWN_KING(b);
	cch_square_t pairs[4][2] = {
		{ CCH_NORTH(k), CCH_SOUTH(k) },
		{ CCH_EAST(k), CCH_WEST(k) },
		{ CCH_NORTHWEST(k), CCH_SOUTHEAST(k) },
		{ CCH_NORTHEAST(k), CCH_SOUTHWEST(k) },
	};

	for(unsigned char i = 0; i < 4; ++i) {
		if(!CCH_IS_SQUARE_VALID(pairs[i][0]) || !CCH_IS_SQUARE_VALID(pairs[i][1])) continue;
		if(!CCH_IS_OWN_PIECE(b, CCH_GET_SQUARE(b, pairs[i][0])) || !CCH_IS_OWN_PIECE(b, CCH_GET_SQUARE(b, pairs[i][1]))) continue;

		/* Both squares should be retreat squares, ie not under check */
		cch_movelist_t ml;
		unsigned char stop = cche_enemy_takers_of_square(b, pairs[i][0], ml, CCH_PSEUDO_LEGAL);
		if(stop > 0) continue;
		stop = cche_enemy_takers_of_square(b, pairs[i][1], ml, CCH_PSEUDO_LEGAL);
		if(stop > 0) continue;

		p->tags.checkmate_epaulette = true;
		return;
	}
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
		if(st->reply.start == 255) {
			cch_play_legal_move(b, &st->move, &um);
			if(CCH_IS_OWN_KING_CHECKED(b)) {
				/* Checkmate leaf */
				p->tags.checkmate_piece |= (1 << (CCH_PURE_PIECE(CCH_GET_SQUARE(b, st->move.end)) - 1));
				tags_smothered_mate(p, st, b);
				tags_suffocation_mate(p, st, b);
				tags_back_rank_mate(p, st, b);
				tags_bodens_mate(p, st, b);
				tags_grecos_mate(p, st, b);
				tags_anastasias_mate(p, st, b);
				tags_arabian_mate(p, st, b);
				tags_corner_mate(p, st, b);
				tags_tail_mate(p, st, b);
				tags_epaulette_mate(p, st, b);
			}
			cch_undo_move(b, &st->move, &um);
			return;
		}

		tags_undermining(p, st, b);

		cch_play_legal_move(b, &(st->move), &um);

		tags_mate_threat(p, b);
		tags_fork(p, st, b);
		tags_skewer(p, st, b);
		tags_pin(p, b, &(st->move));
		tags_trapped_piece(p, st, b);
		tags_discovered_attack(p, st, b);
		tags_overloaded_piece(p, st, b);
		tags_deflection(p, st, b);

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
