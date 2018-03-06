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

const orm_array_shuffle = function(a) {
	/* Knuth shuffle algorithm */
	for(let i = a.length - 1; i >= 1; --i) {
		let j = Math.floor(Math.random() * (i+1));
		let temp = a[j];
		a[j] = a[i];
		a[i] = temp;
	}
};

const orm_generate_endgame = function(s, tries) {
	if(typeof(tries) === "undefined") tries = 0;
	else ++tries;

	if(tries >= 50) {
		/* Too many tries, position is likely unachievable */
		return false;
	}

	let board = [], placed = [];
	for(let i = 0; i < 64; ++i) {
		board.push('1'); /* FEN-speak for "empty square" */
	}

	const parse_filter = function(filter, at) {
		let ret = [];
		let mode = 'push', push = null;

		while(filter !== '') {
			lsw:
			switch(filter[0]) {
			case '*':
				return [ 0, 1, 2, 3, 4, 5, 6, 7 ];

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				push = filter[0].charCodeAt(0) - '0'.charCodeAt(0);
				filter = filter.substring(1);
				break;

			case '@':
				push = at;
				filter = filter.substring(1);
				break;

			case 'F':
			case 'R':
				if(filter.length < 2) return false;
				for(let i = placed.length - 1; i >= 0; --i) {
					if(placed[i][0] !== filter[1]) continue;
					push = filter[0] === 'F' ? (placed[i][1] >> 3) : (placed[i][1] & 7);
					filter = filter.substring(2);
					break lsw;
				}
				return false; /* Piece not found */

			case '+':
				mode = 'plus';
				filter = filter.substring(1);
				continue;

			case '-':
				mode = 'minus';
				filter = filter.substring(1);
				continue;
			}

			if(push === null) return false;
			if(mode === 'push') {
				ret.push(push);
			} else if(mode === 'plus') {
				if(ret === []) return false;
				ret.push(ret.pop() + push);
				mode = 'push';
			} else if(mode === 'minus') {
				if(ret === []) return false;
				ret.push(ret.pop() - push);
				mode = 'push';
			} else {
				return false;
			}
		}

		return ret;
	};

	let ps = s, piece, ranks, files, msquares, squares;
	outer:
	while(ps !== '') {
		let piece = ps[0], ranks = [], files = [], msquares = {}, squares = [];

		if(ps.length > 1 && ps[1] === "{") {
			/* Position filter */
			let m = ps.match(/^[a-zA-Z]\{([^}]+)\}/);
			let filters = m[1].split(',');
			for(let i in filters) {
				let f = filters[i].split('|'), ret, ret2;
				if(f.length !== 2) return false;
				if(f[0].indexOf('@') !== -1 && f[1].indexOf('@') !== -1) return false; /* Unsupported */

				/* XXX: refactor this in a prettier way */
				if(f[1].indexOf('@') !== -1) {
					/* File first */
					if((ret = parse_filter(f[0])) === false) return false;
					for(let j in ret) {
						if((ret2 = parse_filter(f[1], ret[j])) === false) return false;
						files.push([ ret[j] ]);
						ranks.push(ret2);
					}
				} else if(f[0].indexOf('@') !== -1) {
					/* Rank first */
					if((ret = parse_filter(f[1])) === false) return false;
					for(let j in ret) {
						if((ret2 = parse_filter(f[0], ret[j])) === false) return false;
						ranks.push([ ret[j] ]);
						files.push(ret2);
					}
				} else {
					/* Doesn't matter */
					if((ret = parse_filter(f[0])) === false) return false;
					files.push(ret);
					if((ret = parse_filter(f[1])) === false) return false;
					ranks.push(ret);
				}
			}
			ps = ps.substring(m[0].length);
		} else {
			/* No filter: default to {*|*} */
			files.push([ 0, 1, 2, 3, 4, 5, 6, 7 ]);
			ranks.push([ 0, 1, 2, 3, 4, 5, 6, 7 ]);
			ps = ps.substring(1);
		}

		/* Generate possible squares */
		for(let i in ranks) {
			for(let j in ranks[i]) {
				if(ranks[i][j] < 0 || ranks[i][j] > 7) continue;
				for(let k in files[i]) {
					if(files[i][k] < 0 || files[i][k] > 7) continue;
					let sq = 8 * files[i][k] + ranks[i][j];
					if(sq in msquares) continue;
					msquares[sq] = true;
					squares.push(sq);
				}
			}
		}
		if(squares === []) return false;

		/* Try to place the piece */
		orm_array_shuffle(squares);
		for(let i in squares) {
			if(board[squares[i]] !== "1") continue;
			if((piece === "p" || piece === "P") && ((squares[i] & 7) === 0 || (squares[i] & 7) === 7)) continue;
			if(piece === "p" || piece === "P") console.log(piece, squares[i], squares[i] & 7);
			board[squares[i]] = piece;
			placed.push([ piece, squares[i] ]);
			continue outer;
		}

		/* Couldn't place piece, no room left */
		return false;
	}

	let side = Math.random() > .5 ? 'b' : 'w';
	if(side === 'b') {
		/* Flip the board colors */
		let swap = function(p) {
			if(p.toUpperCase() === p) return p.toLowerCase();
			return p.toUpperCase();
		};

		for(let i = 0; i < 32; ++i) {
			let temp = board[i];
			board[i] = swap(board[63 - i]);
			board[63 - i] = swap(temp);
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

	let ownK = board.indexOf(side === 'w' ? 'K' : 'k');
	if(ownK === -1) return false; /* No king?! */
	if(gumble_is_square_checked(ownK)) {
		/* Position is not quiet */
		return orm_generate_endgame(s, tries);
	}

	/* FEN canonicalization, gumble will happily parse "11R11111" as
	 * "2R5", but that's not really spec-compliant */
	return gumble_save_fen();
};

const orm_practice_fen = function(fen) {
	let b = orm_get_board();
	if(orm_practice !== false) $("button#engine-practice").click();

	orm_load_tab("board", true, function() {
		orm_load_fen(fen, orm_get_board());
		$("button#engine-practice").click();
	});
};

orm_when_ready.push(function() {
	$("div#endgames button[data-endgame]").click(function() {
		let a = $(this);
		let fen = orm_generate_endgame(a.data('endgame'));
		if(fen === false) orm_error("Could not generate endgame for " + a.data('endgame') + ".");
		orm_practice_fen(fen);
	});

	$("div#endgames form").submit(function(e) {
		let f = $(this);
		let i = f.children('input');
		let fen = orm_generate_endgame(i.val());
		e.preventDefault();
		if(fen === false) {
			i.addClass('is-invalid');
			return;
		}
		i.removeClass('is-invalid');
		orm_practice_fen(fen);
	});
});
