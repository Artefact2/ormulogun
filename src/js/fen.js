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

const orm_get_board = function(b) {
	if(typeof(b) === "undefined" || b.length === 0) return $("div#board div.board.board-main");
	else return b;
};

const orm_load_fen = function(fen, b) {
	let p, r = 8, f = 1, cl;
	b.children("div.piece").remove();

	for(let i = 0; i < fen.length && fen[i] != " "; ++i) {
		switch(fen[i]) {
		case '/':
			f = 1;
			--r;
			continue;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
			f += parseInt(fen[i], 10);
			continue;

		case 'p': cl = 'piece black pawn'; break;
		case 'n': cl = 'piece black knight'; break;
		case 'b': cl = 'piece black bishop'; break;
		case 'r': cl = 'piece black rook'; break;
		case 'q': cl = 'piece black queen'; break;
		case 'k': cl = 'piece black king'; break;

		case 'P': cl = 'piece white pawn'; break;
		case 'N': cl = 'piece white knight'; break;
		case 'B': cl = 'piece white bishop'; break;
		case 'R': cl = 'piece white rook'; break;
		case 'Q': cl = 'piece white queen'; break;
		case 'K': cl = 'piece white king'; break;

		default: cl = false; break;
		}

		if(cl === false || r < 1 || f > 8) {
			orm_error("Error parsing FEN: " + fen);
			return;
		}

		p = $(document.createElement('div'));
		p.addClass(cl + " f" + f + " r" + r);
		p.data('ofile', f).data('orank', r);
		b.append(p);

		++f;
	}

	let side = fen.split(" ", 3)[1];
	if(side === "w") {
		b.removeClass("black").addClass("white");
	} else {
		b.removeClass("white").addClass("black");
	}

	b.data('fen', fen);
	if(b.hasClass('board-main')) {
		$("div#board-fen > form > input").val(fen);
	}
};


const orm_file = function(alg) {
	return alg.charCodeAt(0) - "a".charCodeAt(0) + 1;
};

const orm_rank = function(alg) {
	return alg.charCodeAt(1) - "1".charCodeAt(0) + 1;
};

const orm_alg = function(file, rank, file2, rank2) {
	if(typeof(file) === "object" && typeof(rank) === "object") {
		return orm_alg(file) + orm_alg(rank);
	}
	if(typeof(file) === "object" && typeof(rank) === "undefined") {
		return orm_alg(file.data("ofile"), file.data("orank"));
	}
	if(typeof(file2) !== "undefined" && typeof(rank2) !== "undefined") {
		return orm_alg(file, rank) + orm_alg(file2, rank2);
	}

	return String.fromCharCode(
		"a".charCodeAt(0) + file - 1,
		"1".charCodeAt(0) + rank - 1
	);
};

const orm_piece_at = function(alg, cl, b) {
	if(!cl) cl = "piece";
	cl += ".f" + orm_file(alg);
	cl += ".r" + orm_rank(alg);
	return b.children("div." + cl);
};

const orm_sq = function(file, rank) {
	return (file - 1) * 8 + (rank - 1);
};

orm_when_ready.push(function() {
	$("div#board-fen > form").submit(function() {
		let f = $(this).children('input').val();
		orm_load_fen(f, orm_get_board());
	});
});
