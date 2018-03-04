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

const orm_square_color = function(sq) {
	return ((sq & 7) + (sq >> 3)) & 1;
};

const orm_generate_endgame = function(s) {
	let side = Math.random() > .5 ? "w" : "b";
	let ss = s.split('/', 2);
	if(side === "w") {
		ss[1] = ss[1].toLowerCase();
	} else {
		ss[0] = ss[0].toLowerCase();
	}

	let squares = [], sqidx = 0, board = [];
	for(let i = 0; i < 64; ++i) {
		squares.push(i);
		board.push('1'); /* FEN-speak for "empty square" */
	}
	/* Knuth shuffle algorithm */
	for(let i = 63; i >= 1; --i) {
		let j = Math.floor(Math.random() * (i+1));
		let temp = squares[j];
		squares[j] = squares[i];
		squares[i] = temp;
	}

	let eking = null;
	let bishops = [ null, null ];
	for(let i = 0; i < 2; ++i) {
		let len = ss[i].length;
		for(let j = 0; j < len; ++j) {
			let p = ss[i][j].toUpperCase();

			/* Don't put pawns on the first or last rank */
			while(p === "P" && (squares[sqidx] % 8 === 0 || squares[sqidx] % 8 === 7)) ++sqidx;

			/* Put bishops on opposite colors */
			while(p === "B" && bishops[i] !== null && orm_square_color(squares[sqidx]) === orm_square_color(bishops[i])) ++sqidx;

			board[squares[sqidx]] = ss[i][j];
			if(p === "B") bishops[i] = squares[sqidx];
			if(p === "K" && i === 1) eking = squares[sqidx];
			++sqidx;
		}
	}

	let fen = '';
	for(let r = 7; r >= 0; --r) {
		if(fen !== '') fen += '/';
		for(let f = 0; f < 8; ++f) {
			fen += board[8 * f + r];
		}
	}
	fen += ' ' + side + ' - -';
	gumble_load_fen(fen);

	/* Is the position legal? Check if we can "take" the enemy king or have no legal moves
	 * (XXX: this relies on gumble movegen internals) */
	let stop = Module._cch_generate_moves(gumble_board, gumble_movelist, 0, 0, 64); /* XXX: refactor me */
	if(stop === 0) {
		/* Checkmate or stalemate, this position is stupid */
		return orm_generate_endgame(s);
	}
	for(let i = 0; i < stop; ++i) {
		/* XXX */
		Module._cch_format_lan_move(gumble_movelist + 4 * i, gumble_move_str, GUMBLE_SAFE_ALG_LENGTH);
		let dest = Pointer_stringify(gumble_move_str).substring(2);
		if(orm_sq(orm_file(dest), orm_rank(dest)) === eking) {
			/* Enemy king is already in check and it is our turn! Illegal position. */
			return orm_generate_endgame(s);
		}
	}

	/* FEN canonicalization, gumble will happily parse "11R11111" as
	 * "2R5", but that's not really spec-compliant */
	return gumble_save_fen();
};

orm_when_ready.push(function() {
	$("div#endgames a[data-endgame]").click(function() {
		let a = $(this);
		a.prop('href', 'https://lichess.org/analysis/' + orm_generate_endgame(a.data('endgame')).replace(/\s/g, '_'));
	});

	$("div#endgames form").submit(function(e) {
		let f = $(this);
		let i = f.children('input');
		let v = i.val();
		if(!v || v.indexOf('/') === -1) {
			e.preventDefault();
			i.addClass('is-invalid');
			return;
		}
		i.removeClass('is-invalid');
		f.prop('action', 'https://lichess.org/analysis/' + orm_generate_endgame(v).replace(/\s/g, '_'));
	});
});
