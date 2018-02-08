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

let orm_candidate_move = null;

const orm_do_legal_move = function(lan, animate, done, pushhist) {
	let psrc = orm_piece_at(lan.substring(0, 2));
	let pdest = orm_piece_at(lan.substring(2));

	let work = function() {
		gumble_play_legal_lan(lan);
		orm_load_fen(gumble_save_fen());
		if(done) done();
	};

	if(typeof(pushhist) === "undefined" || pushhist) {
		orm_movehist_push(gumble_save_fen(), lan, gumble_lan_to_san(lan));
	}

	if(animate) {
		psrc.removeClass("f" + psrc.data("ofile") + " r" + psrc.data("orank"));
		psrc.addClass("moving f" + orm_file(lan.substring(2)) + " r" + orm_rank(lan.substring(2)));
		pdest.addClass("captured");
		setTimeout(function() {
			work();
		}, 500);
	} else {
		work();
	}
};

const orm_do_puzzle_move = function(lan, animate, done) {
	/* XXX: promote */
	if(!gumble_is_move_legal(lan)) return;
	orm_do_legal_move(lan, animate, function() {
		orm_puzzle_try(lan);
		if(done) done();
	});
};

const orm_can_move_piece = function(p) {
	let b = $("div#board");
	return (p.hasClass("white") === b.hasClass("white")) && (p.hasClass("black") === b.hasClass("black"));
};

const orm_highlight_move_squares = function(sf, sr) {
	let sq = (sf - 1) * 8 + (sr - 1);
	let b = $("div#board");

	let stop = Module._cch_generate_moves(gumble_board, gumble_movelist, 0, sq, sq + 1);

	b.children('div.back.f' + sf + '.r' + sr).addClass('move-source');
	for(let i = 0; i < stop; ++i) {
		Module._cch_format_lan_move(gumble_movelist + 4 * i, gumble_move_str, GUMBLE_SAFE_ALG_LENGTH);
		let dest = Pointer_stringify(gumble_move_str).substring(2);
		b.children('div.back.f' + orm_file(dest) + '.r' + orm_rank(dest)).addClass('move-target');
	}
};

orm_when_ready.push(function() {
	$("button#flip-board").click(function() {
		$("div#board").toggleClass('flipped');
	});

	/* XXX not touch-friendly */
	$("div#board").on("mousedown", "> div.piece", function(e) {
		let p = $(this);
		if(!orm_can_move_piece(p)) return;

		let bg = $("div#board > div.back.r" + p.data("orank") + ".f" + p.data("ofile"));
		p.data("origx", e.pageX);
		p.data("origy", e.pageY);
		p.data("origtop", p.position().top);
		p.data("origleft", p.position().left);
		p.addClass("dragging");
	}).on("mousemove", function(e) {
		let p = $("div#board > div.piece.dragging");
		if(p.length === 0) return;

		if(!p.hasClass("dragged")) {
			p.addClass("dragged");
			orm_highlight_move_squares(p.data('ofile'), p.data('orank'));
		}
		p.css("left", p.data("origleft") + e.pageX - p.data("origx"));
		p.css("top", p.data("origtop") + e.pageY - p.data("origy"));
	}).on("mouseup", "> div.piece.dragging", function(e) {
		let p = $(this);
		let b = $("div#board"), pos = p.position();
		let file = 1 + Math.round(8 * pos.left / b.width());
		let rank = 9 - Math.round(1 + 8 * pos.top / b.height());

		if($("div#board").hasClass('flipped')) {
			file = 9 - file;
			rank = 9 - rank;
		}

		p.removeAttr('style');
		p.removeClass('dragging');
		$("div#board > div.back").removeClass("move-source move-target");

		orm_do_puzzle_move(orm_alg(p) + orm_alg(file, rank), false);
	}).on("click", "> div", function() {
		let p = $(this);
		if(p.hasClass("dragged")) {
			orm_candidate_move = null;
			p.removeClass("dragged");
			return;
		}

		if(orm_candidate_move === null) {
			if(p.hasClass("back") || (p.hasClass("piece") && !orm_can_move_piece(p))) return;
			let bg = $("div#board > div.back.r" + p.data("orank") + ".f" + p.data("ofile"));
			orm_candidate_move = orm_alg(p);
			orm_highlight_move_squares(p.data('ofile'), p.data('orank'));
		} else {
			$("div#board > div.back").removeClass("move-source move-target");
			let tgt = orm_alg(p);
			if(tgt !== orm_candidate_move) {
				orm_do_puzzle_move(orm_candidate_move + tgt, true);
			}
			orm_candidate_move = null;
		}
	});
});
