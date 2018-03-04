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

const orm_generate_endgame = function(s, tries) {
	if(typeof(tries) === "undefined") tries = 0;
	else ++tries;

	if(tries >= 50) {
		/* Too many tries, position is likely unachievable */
		return false;
	}

	let side = Math.random() > .5 ? "w" : "b";
	let ss = s.split('/', 2);
	if(ss.length !== 2) return false;

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

	let stop = Module._cch_generate_moves(gumble_board, gumble_movelist, 0, 0, 64); /* XXX: refactor me */
	if(stop === 0) {
		/* Checkmate or stalemate, this position is stupid */
		return orm_generate_endgame(s, tries);
	}
	for(let i = 0; i < stop; ++i) {
		/* XXX */
		Module._cch_format_lan_move(gumble_movelist + 4 * i, gumble_move_str, GUMBLE_SAFE_ALG_LENGTH);
		let dest = Pointer_stringify(gumble_move_str).substring(2);
		if(board[orm_sq(orm_file(dest), orm_rank(dest))] !== '1') {
			/* Position is not quiet */
			return orm_generate_endgame(s, tries);
		}
	}

	let k = board.indexOf('k');
	let K = board.indexOf('K');
	if(k === -1 || K === -1) return false; /* No kings?! */
	let dist = Math.abs((k & 7) - (K & 7)) + Math.abs(((k >> 3) & 7) - ((K >> 3) & 7));
	if(dist <= 2) {
		/* Kings are in an illegal position */
		return orm_generate_endgame(s, tries);
	}

	/* FEN canonicalization, gumble will happily parse "11R11111" as
	 * "2R5", but that's not really spec-compliant */
	return gumble_save_fen();
};

orm_when_ready.push(function() {
	$("div#endgames a[data-endgame]").click(function() {
		let a = $(this);
		let fen = orm_generate_endgame(a.data('endgame'));
		if(fen === false) orm_error("Could not generate endgame for " + a.data('endgame') + ".");
		a.prop('href', 'https://lichess.org/analysis/' + fen.replace(/\s/g, '_'));
	});

	$("div#endgames form").submit(function(e) {
		let f = $(this);
		let i = f.children('input');
		let fen = orm_generate_endgame(i.val());
		if(fen === false) {
			e.preventDefault();
			i.addClass('is-invalid');
			return;
		}
		i.removeClass('is-invalid');
		f.prop('action', 'https://lichess.org/analysis/' + fen.replace(/\s/g, '_'));
	});
});
