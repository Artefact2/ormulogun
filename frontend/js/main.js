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

let orm_manifest = null;
let orm_puzzle_set = null;
let orm_puzzle_midx = null;
let orm_puzzle_idx = null;
let orm_puzzle = null;
let orm_puzzle_next = null;
let orm_movelist = null;
let orm_movelist_idx = null;
let orm_candidate_move = null;

let orm_error = function(str) {
	let err = $(document.createElement('p'));
	err.addClass('alert alert-danger');
	err.text(str);
	$("nav#mainnav").after(err);
};

let orm_load_puzzle_manifest = function(done) {
	$.getJSON("./puzzles/manifest.js").always(function() {
		$("div#select-puzzleset > p.alert").remove();
	}).fail(function() {
		orm_error('Could not load the puzzle set manifest, make sure no extension is blocking XHR.');
	}).done(function(data) {
		orm_manifest = data;

		let ul = $("div#select-puzzleset > ul");
		let hash = location.hash.split('-', 3);
		for(let i in data) {
			let pset = data[i];
			let li = $(document.createElement('li'));
			li.addClass('row pb-2');

			let h = $(document.createElement('h3'));
			h.text(pset.name);
			h.addClass('col');
			li.append(h);

			let p = $(document.createElement('p'));
			p.text(pset.desc + " " + pset.count + " puzzles.");
			p.addClass('col-6');
			li.append(p);

			let btn = $(document.createElement('button'));
			btn.text('Start training');
			btn.addClass('btn btn-primary col start-training');
			btn.data('idx', i);
			li.append(btn);

			ul.append(li);
		}

		ul.find("button.start-training").click(function() {
			let t = $(this);
			t.prop('disabled', 'disabled');
			t.addClass('disabled');
			orm_load_puzzle_set(t.data('idx'), null, null, function() {
				$("div#select-puzzleset").fadeOut(250, function() {
					orm_load_puzzle(0);
					$("div#play-puzzle").fadeIn(250);
				});
			});
		});

		done();
	});
};

let orm_load_puzzle_set = function(m_idx, always, fail, done) {
	let p = $(document.createElement('p'));
	p.addClass("alert alert-primary").text("Loading the puzzle set…");
	$("nav#mainnav").after(p);

	$.getJSON("./puzzles/" + orm_manifest[m_idx].src.replace(/\.json$/, '.js')).always(function() {
		p.remove();
		if(always) always();
	}).fail(function() {
		orm_error("Could not load the puzzle set : " + orm_manifest[m_idx].id + ".");
		if(fail) fail();
	}).done(function(data) {
		orm_puzzle_set = data;
		orm_puzzle_midx = m_idx;
		if(done) done();
	});
};

let orm_restore_tab = function() {
	$("div#intro").hide();
	let h = location.hash.split("-", 3);
	switch(h[0]) {
	case "#list":
		$("div#select-puzzleset").show();
		return;

	case "#puzzle":
		/* XXX */
		if(h.length !== 3) break;
		let manifest_idx = null;
		for(let i in orm_manifest) {
			if(orm_manifest[i].id === h[1]) {
				manifest_idx = i;
				break;
			}
		}
		if(manifest_idx === null) break;
		orm_load_puzzle_set(manifest_idx, null, function() {
			$("div#intro").show();
		}, function() {
			let idx = parseInt(h[2], 10);
			orm_load_puzzle(idx);
			$("div#play-puzzle").show();
		});
		return;
	}

	$("div#intro").show();
};

let orm_init_board = function() {
	let b = $("div#board");
	let d;
	for(let f = 1; f <= 8; ++f) {
		for(let r = 1; r <= 8; ++r) {
			d = $(document.createElement('div'));
			d.addClass("back r" + r + " f" + f + " " + ((f + r) % 2 ? 'light' : 'dark'));
			d.data("ofile", f);
			d.data("orank", r);
			b.append(d);
		}
	}
};

let orm_load_fen = function(fen) {
	let b = $("div#board");
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
}

let orm_animate_move = function(startfen, movelan, endfen, start, done, delay) {
	if(startfen) orm_load_fen(startfen);

	let sf, sr, ef, er, cl;
	sf = movelan.charCodeAt(0) - "a".charCodeAt(0) + 1;
	sr = movelan.charCodeAt(1) - "1".charCodeAt(0) + 1;
	ef = movelan.charCodeAt(2) - "a".charCodeAt(0) + 1;
	er = movelan.charCodeAt(3) - "1".charCodeAt(0) + 1;
	/* XXX: promotion */

	let psrc = $("div#board > div.piece.f" + sf + ".r" + sr);
	let pdest = $("div#board > div.piece.f" + ef + ".r" + er);

	setTimeout(function() {
		psrc.removeClass("f" + sf + " r" + sr);
		psrc.addClass("moving f" + ef + " r" + er);
		psrc.data("ofile", ef).data("orank", er);
		pdest.addClass('captured');
		setTimeout(function() {
			pdest.remove();
			psrc.removeClass("moving");
			if(endfen) orm_load_fen(endfen);
			if(done) done();
		}, 500);
		if(start) start();
	}, typeof(delay) === "undefined" ? 750 : delay);
};

let orm_push_move = function(startfen, movesan, movelan, endfen) {
	if(orm_movelist !== null && orm_movelist_idx !== orm_movelist.length - 1) {
		orm_error("Can't push a move in this position.");
		return;
	}

	if(orm_movelist === null) {
		orm_movelist = [];
		orm_movelist_idx = -1;
	}

	++orm_movelist_idx;
	orm_movelist.push([ startfen, movesan, movelan, endfen ]);

	$("table#movelist button.btn-primary").removeClass("btn-primary").addClass("btn-light");

	let btn = $(document.createElement('button'));
	btn.text(movesan);
	btn.data('idx', orm_movelist_idx);
	btn.attr("id", "ply" + orm_movelist_idx);
	btn.addClass('btn btn-sm btn-block btn-primary');

	let tb = $("table#movelist > tbody");
	let tr = tb.children('tr').last();
	let td;
	if(tr.length === 0 || tr.find('button').length === 2) {
		tr = $(document.createElement("tr"));
		tb.append(tr);

		let th = $(document.createElement("th"));
		th.text(Math.floor((orm_puzzle.ply + orm_movelist.length) / 2) + ".");
		tr.append(th);

		td = $(document.createElement("td"));
		tr.append(td);
		let td2 = $(document.createElement("td"));
		tr.append(td2);

		if((orm_puzzle.ply  + orm_movelist.length) % 2) {
			let dummy = $(document.createElement("button"));
			dummy.text("...");
			dummy.addClass("btn btn-sm btn-block btn-light disabled");
			dummy.prop("disabled", "disabled");
			td.append(dummy);
			td = td2;
		}
	} else {
		td = tr.children("td").last();
	}

	td.append(btn);
};

let orm_load_puzzle = function(idx) {
	orm_puzzle_idx = idx;
	orm_movelist = orm_movelist_idx = null;
	let puz = orm_puzzle = orm_puzzle_set[idx];

	$("table#movelist > tbody").empty();
	$("a#puzzle-analysis").prop('href', 'https://lichess.org/analysis/' + puz[0].replace(/\s/g, '_'));
	history.replaceState(null, null, "#puzzle-" + orm_manifest[orm_puzzle_midx].id + "-" + idx);

	puz.side = puz[0].split(' ', 3)[1] === 'b';
	$("div#board").toggleClass('flipped', !puz.side);

	/* XXX: can be refactored? get rid of 1st parameter to orm_animate_move */
	const fen = puz[0];
	gumble_load_fen(fen);
	const lan = puz[1];
	const san = gumble_lan_to_san(lan);
	gumble_play_legal_lan(lan);
	const afterfen = gumble_save_fen();
	orm_animate_move(fen, lan, afterfen, function() {
		orm_push_move(fen, san, lan, afterfen);
	});

	$("p#puzzle-prompt")
		.removeClass("text-success text-danger")
		.text(orm_manifest[orm_puzzle_midx].prompt.replace("{%side}",  puz.side ? 'White' : 'Black'));
	$("div#puzzle-actions-after").removeClass("visible");
	$("button#puzzle-abandon").show();
	$("button#puzzle-next").hide();
	$("nav#mainnav").removeClass("bg-success bg-danger");
	orm_puzzle_next = puz[2];
};

let orm_puzzle_over = function() {
	$("div#puzzle-actions-after").addClass('visible');
	$("button#puzzle-abandon").hide();
	$("button#puzzle-next").show();
	orm_puzzle_next = null;
};

let orm_puzzle_success = function() {
	$("p#puzzle-prompt").addClass("text-success").text("Puzzle completed successfully!");
	$("nav#mainnav").addClass("bg-success");
	orm_puzzle_over();
};

let orm_puzzle_fail = function() {
	let oldidx = orm_movelist_idx;
	let oldbtn = $("table#movelist button.btn-primary");
	while(orm_puzzle_next !== null) {
		for(let lan in orm_puzzle_next) {
			let puz = orm_puzzle_next[lan];
			orm_push_move(orm_movelist[orm_movelist_idx][3], puz.move.san, lan, puz.move.fen); /* XXX */
			orm_push_move(puz.move.fen, puz.reply.san, puz.reply.lan, puz.reply.fen);
			orm_puzzle_next = puz.next;
			break;
		}
	}
	/* XXX */
	orm_movelist_idx = oldidx;
	$("table#movelist button.btn-primary").removeClass("btn-primary").addClass("btn-light");
	oldbtn.removeClass("btn-light").addClass("btn-primary");

	$("p#puzzle-prompt").addClass("text-danger").text("Puzzle failed.");
	$("nav#mainnav").addClass("bg-danger");
	orm_puzzle_over();
};

let orm_can_move_piece = function(p) {
	let b = $("div#board");
	if(p.hasClass("white") !== b.hasClass("white") || p.hasClass("black") !== b.hasClass("black")) return false;
	if(orm_puzzle !== null && orm_puzzle_next !== null && p.hasClass("white") !== orm_puzzle.side) return false;
	if(orm_puzzle_next !== null && orm_movelist_idx !== orm_movelist.length - 1) return false;
	return true;
};

let orm_do_user_move = function(lan, animate) {
	/* XXX: promote */

	let sf, sr, tf, tr;
	sf = lan.charCodeAt(0) - "a".charCodeAt(0) + 1;
	sr = lan.charCodeAt(1) - "1".charCodeAt(0) + 1;
	tf = lan.charCodeAt(2) - "a".charCodeAt(0) + 1;
	tr = lan.charCodeAt(3) - "1".charCodeAt(0) + 1;

	if(sf < 1 || sf > 8 || sr < 1 || sr > 8 || tf < 1 || tf > 8 || tr < 1 || tr > 8) return false;
	if(sf === tf && sr === tr) return false;

	let b = $("div#board");
	gumble_load_fen(b.data('fen'));
	if(!gumble_is_move_legal(lan)) {
		return false;
	}
	b.toggleClass("white black");

	let after = function() {
		if(orm_puzzle_next === null) {
			gumble_play_legal_lan(lan);
			orm_load_fen(gumble_save_fen());
			/* XXX: continue pushing moves */
			return true;
		}

		if(lan in orm_puzzle_next) {
			const puz = orm_puzzle_next[lan];
			const fen = b.data('fen');
			gumble_load_fen(fen);
			const san = gumble_lan_to_san(lan);
			gumble_play_legal_lan(lan);
			const fen2 = gumble_save_fen();
			b.data('fen', fen2);
			orm_push_move(fen, san, lan, fen2);

			if(puz === null) {
				orm_puzzle_success();
			} else {
				$("p#puzzle-prompt").text("Good move! Keep going…");

				const rlan = puz[0];
				const rsan = gumble_lan_to_san(rlan);
				gumble_play_legal_lan(rlan);
				const rfen = gumble_save_fen();

				orm_animate_move(fen, lan, rfen, function() {
					orm_push_move(fen2, rsan, rlan, rfen);
				});
				orm_puzzle_next = puz[1];
			}
		} else {
			orm_puzzle_fail();
		}
	};

	if(animate) {
		orm_animate_move(null, lan, null, null, after, 0);
	} else {
		b.children("div.piece.r" + tr + ".f" + tf).remove();

		let p = b.children("div.piece.r" + sr + ".f" + sf);
		p.removeClass("r" + sr + " f" + sf);
		p.addClass("r" + tr + " f" + tf);
		p.data("orank", tr).data("ofile", tf);

		after();
	}

	return true;
};

let orm_highlight_move_squares = function(sf, sr) {
	let sq = (sf - 1) * 8 + (sr - 1);
	let b = $("div#board");

	gumble_load_fen(b.data('fen'));
	let stop = Module._cch_generate_moves(gumble_board, gumble_movelist, 0, sq, sq + 1);

	b.children('div.back.f' + sf + '.r' + sr).addClass('move-source');
	for(let i = 0; i < stop; ++i) {
		Module._cch_format_lan_move(gumble_movelist + 4 * i, gumble_move_str, GUMBLE_SAFE_ALG_LENGTH);
		let lan = Pointer_stringify(gumble_move_str);
		b.children('div.back.f' + (lan.charCodeAt(2) - "a".charCodeAt(0) + 1) + '.r' + (lan.charCodeAt(3) - "1".charCodeAt(0) + 1)).addClass('move-target');
	}
};

$(function() {
	$("p#enable-js").remove();

	for(let i in when_ready) when_ready[i]();

	$("button#start").click(function() {
		$("div#intro").fadeOut(250, function() {
			$("div#select-puzzleset").fadeIn(250);
			history.replaceState(null, null, "#list");
		});
	});

	$("button#flip-board").click(function() {
		$("div#board").toggleClass('flipped');
	});

	$("button#puzzle-retry").click(function() {
		orm_load_puzzle(orm_puzzle_idx);
	});

	$("button#puzzle-abandon").click(function() {
		orm_puzzle_fail();
	});

	$("button#puzzle-next").click(function() {
		/* XXX */
		orm_load_puzzle(orm_puzzle_idx + 1);
	});

	$("table#movelist").on("click", "button", function() {
		let b = $(this);
		orm_movelist_idx = b.data("idx");
		let m = orm_movelist[orm_movelist_idx];
		$("table#movelist button.btn-primary").removeClass("btn-primary").addClass("btn-light");
		b.removeClass("btn-light").addClass("btn-primary");
		orm_animate_move(m[0], m[2], m[3], null, null, 100);
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

		/* XXX: promotion */
		orm_do_user_move(String.fromCharCode(
			"a".charCodeAt(0) + p.data("ofile") - 1,
			"1".charCodeAt(0) + p.data("orank") - 1,
			"a".charCodeAt(0) + file - 1,
			"1".charCodeAt(0) + rank - 1
		), false);

		p.removeAttr('style');
		p.removeClass('dragging');
		$("div#board > div.back").removeClass("move-source move-target");
	}).on("click", "> div", function() {
		let p = $(this);
		if(p.hasClass("dragged")) {
			orm_candidate_move = null;
			p.removeClass("dragged");
			return;
		}
		if(orm_candidate_move === null && (p.hasClass("back") || (p.hasClass("piece") && !orm_can_move_piece(p)))) return;

		if(orm_candidate_move === null) {
			let bg = $("div#board > div.back.r" + p.data("orank") + ".f" + p.data("ofile"));
			orm_candidate_move = String.fromCharCode(
				"a".charCodeAt(0) + p.data("ofile") - 1,
				"1".charCodeAt(0) + p.data("orank") - 1,
			);
			orm_highlight_move_squares(p.data('ofile'), p.data('orank'));
		} else {
			$("div#board > div.back").removeClass("move-source move-target");

			let tgt = String.fromCharCode(
				"a".charCodeAt(0) + p.data("ofile") - 1,
				"1".charCodeAt(0) + p.data("orank") - 1,
			);

			if(tgt !== orm_candidate_move) {
				orm_do_user_move(orm_candidate_move + tgt, true);
			}

			orm_candidate_move = null;
		}
	});

	orm_load_puzzle_manifest(function() {
		orm_init_board();
		orm_restore_tab();
	});
});
