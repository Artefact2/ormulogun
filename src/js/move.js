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

let orm_move_timeoutid = null;
let orm_promote_context = null;

const orm_do_legal_move = function(lan, animate, done, pushhist, reverse, b) {
	pushhist = typeof(pushhist) === "undefined" || pushhist;
	reverse = typeof(reverse) !== "undefined" && reverse;

	let startfen = gumble_save_fen(), san;
	if(pushhist) san = gumble_lan_to_san(lan);
	gumble_play_legal_lan(lan);
	if(pushhist) orm_movehist_push(startfen, lan, san);

	let endfen = gumble_save_fen();
	if(reverse) {
		gumble_load_fen(startfen); /* XXX: is it worth using cch_undo_move? */
	}

	let work = function() {
		orm_load_fen(endfen, b);

		if(orm_analyse === true) {
			orm_uci_go();
		}

		/* King check check depends on gumble state, which orm_load_fen is obvlivious to */
		b.children("div.piece.king.in-check").removeClass('in-check');
		let k = b.hasClass('white') ? b.children('div.piece.king.white') : b.children('div.piece.king.black');
		gumble_load_fen(endfen);
		if(gumble_is_own_king_checked()) {
			k.addClass('in-check');
		}

		if(done) done();
	};

	if(!animate) {
		b.children("div.back.move-prev").removeClass('move-prev');
		orm_piece_at(lan.substring(0, 2), 'back', b).addClass('move-prev');
		orm_piece_at(lan.substring(2), 'back', b).addClass('move-prev');
		work();
		return;
	}

	if(reverse) {
		orm_load_fen(endfen, b);
		endfen = startfen;
	} else {
		orm_load_fen(startfen, b);
	}

	let psrc, pdest, df, dr;
	if(reverse) {
		psrc = orm_piece_at(lan.substring(2), null, b);
		pdest = orm_piece_at(lan.substring(0, 2), null, b);
		df = orm_file(lan.substring(0, 2));
		dr = orm_rank(lan.substring(0, 2));
	} else {
		psrc = orm_piece_at(lan.substring(0, 2), null, b);
		pdest = orm_piece_at(lan.substring(2), null, b);
		df = orm_file(lan.substring(2));
		dr = orm_rank(lan.substring(2));
	}

	setTimeout(function() {
		psrc.removeClass("f" + psrc.data("ofile") + " r" + psrc.data("orank"));
		psrc.addClass("moving f" + df + " r" + dr);
		pdest.addClass("captured");
		b.children("div.back.move-prev").removeClass('move-prev');
		if(orm_move_timeoutid !== null) clearTimeout(orm_move_timeoutid);
		orm_move_timeoutid = setTimeout(function() {
			orm_move_timeoutid = null;
			orm_piece_at(lan.substring(0, 2), 'back', b).addClass('move-prev');
			orm_piece_at(lan.substring(2), 'back', b).addClass('move-prev');
			work();
		}, orm_pref("board_animation_speed"));
	}, 50);
};

const orm_do_puzzle_move = function(lan, animate, done, b) {
	let piece = orm_piece_at(lan.substring(0, 2), null, b);
	if(lan.length === 4 && piece.hasClass("pawn")
	   && ((piece.hasClass("white") && orm_rank(lan.substring(2)) === 8)
		   || (piece.hasClass("black") && orm_rank(lan.substring(2)) === 1))) {
		let m = $("div#promote-modal");

		if(piece.hasClass("black")) m.find("div.piece").removeClass('white').addClass('black');
		else m.find("div.piece").removeClass('black').addClass('white');

		orm_promote_context = [ lan, animate, done, b ];
		m.modal('show');
	}
	gumble_load_fen(b.data('fen'));
	if(!gumble_is_move_legal(lan)) return;
	orm_do_legal_move(lan, animate, function() {
		if(b.hasClass('board-main')) {
			orm_puzzle_try(lan);

			if(orm_practice !== false && b.hasClass(orm_practice) && !b.hasClass('game-over')) {
				orm_uci_go_practice();
			}
		}
		if(done) done();
	}, undefined, undefined, b);
};

const orm_can_move_piece = function(p, b) {
	if(b.hasClass('game-over')) return false;
	if(orm_practice === true && b.hasClass(orm_practice)) return false;
	return (p.hasClass("white") === b.hasClass("white")) && (p.hasClass("black") === b.hasClass("black"));
};

const orm_highlight_move_squares = function(sf, sr, b) {
	let sq = (sf - 1) * 8 + (sr - 1);
	gumble_load_fen(b.data('fen'));

	/* XXX: refactor me */
	let stop = Module._cch_generate_moves(gumble_board, gumble_movelist, 0, sq, sq + 1);

	b.children('div.back.f' + sf + '.r' + sr).addClass('move-source');
	for(let i = 0; i < stop; ++i) {
		/* XXX */
		Module._cch_format_lan_move(gumble_movelist + 4 * i, gumble_move_str, GUMBLE_SAFE_ALG_LENGTH);
		let dest = Pointer_stringify(gumble_move_str).substring(2);
		b.children('div.back.f' + orm_file(dest) + '.r' + orm_rank(dest)).addClass('move-target');
	}
};

orm_when_ready.push(function() {
	$("button#flip-board").click(function() {
		orm_get_board().toggleClass('flipped');
	});

	/* XXX not touch-friendly */
	$("div.board").on("mousedown", "> div.piece", function(e) {
		let p = $(this);
		let b = p.parent();
		if(!orm_can_move_piece(p, b)) return;

		let bg = b.children("div.back.r" + p.data("orank") + ".f" + p.data("ofile"));
		p.data("origx", e.pageX);
		p.data("origy", e.pageY);
		p.data("origtop", p.position().top);
		p.data("origleft", p.position().left);
		p.addClass("dragging");
	}).on("mousemove", function(e) {
		let b = $(this);
		let p = b.children("div.piece.dragging");
		if(p.length === 0) return;

		if(!p.hasClass("dragged")) {
			p.addClass("dragged");
			orm_highlight_move_squares(p.data('ofile'), p.data('orank'), b);
		}
		p.css("left", p.data("origleft") + e.pageX - p.data("origx"));
		p.css("top", p.data("origtop") + e.pageY - p.data("origy"));
	}).on("mouseup", "> div.piece.dragging", function(e) {
		let p = $(this);
		let b = p.parent(), pos = p.position();
		let file = 1 + Math.round(8 * pos.left / b.width());
		let rank = 9 - Math.round(1 + 8 * pos.top / b.height());

		if(b.hasClass('flipped')) {
			file = 9 - file;
			rank = 9 - rank;
		}

		p.removeAttr('style');
		p.removeClass('dragging');
		b.children("div.back").removeClass("move-source move-target");

		orm_do_puzzle_move(orm_alg(p) + orm_alg(file, rank), false, null, b);
	}).on("click", "> div", function() {
		let p = $(this);
		let b = p.parent();
		if(p.hasClass("dragged")) {
			b.data('candidate-move', null);
			p.removeClass("dragged");
			return;
		}

		if(!b.data('candidate-move')) {
			if(p.hasClass("back") || (p.hasClass("piece") && !orm_can_move_piece(p, b))) return;
			let bg = b.children("div.back.r" + p.data("orank") + ".f" + p.data("ofile"));
			b.data('candidate-move', orm_alg(p));
			orm_highlight_move_squares(p.data('ofile'), p.data('orank'), b);
		} else {
			b.children("div.back").removeClass("move-source move-target");
			let tgt = orm_alg(p);
			if(tgt !== b.data('candidate-move')) {
				orm_do_puzzle_move(b.data('candidate-move') + tgt, true, null, b);
			}
			b.data('candidate-move', null);
		}
	});

	$("div#promote-modal").modal({
		backdrop: 'static',
		keyboard: false,
		show: false,
		focus: false,
	}).on('hidden.bs.modal', function() {
		orm_do_puzzle_move(orm_promote_context[0], orm_promote_context[1], function() {
			if(typeof(orm_promote_context[2]) === "function") {
				orm_promote_context[2]();
			}
			orm_promote_context = null;
		}, orm_promote_context[3]);
	}).on('click', 'div.piece', function() {
		let p = $(this);
		let append = null;
		if(p.hasClass('queen')) {
			append = 'q';
		} else if(p.hasClass('rook')) {
			append = 'r';
		} else if(p.hasClass('bishop')) {
			append = 'b';
		} else if(p.hasClass('knight')) {
			append = 'n';
		} else return;
		if(orm_promote_context[0].length === 4) {
			orm_promote_context[0] += append;
		}
		$("div#promote-modal").modal('hide');
	});

	$("div#analysis-stuff > ul").on('mouseenter', '> li.pv', function() {
		let li = $(this);
		let b = orm_get_board();
		let src = orm_piece_at(li.data('pv').substr(0, 2), 'back', b);
		let dest = orm_piece_at(li.data('pv').substr(2, 2), 'back', b);
		src.addClass('pv-move-source');
		dest.addClass('pv-move-target');
	}).on('mouseleave', '> li.pv', function() {
		let b = orm_get_board();
		b.children('div.pv-move-source, div.pv-move-target').removeClass('pv-move-source pv-move-target');
	}).on('click', '> li.pv', function() {
		let li = $(this);
		let b = orm_get_board();
		orm_do_puzzle_move(li.data('pv').split(' ', 2)[0], true, null, b);
		li.trigger('mouseleave');
	});
});
