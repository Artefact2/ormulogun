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

"use strict";

var orm_manifest = null;
var orm_puzzle_set = null;
var orm_puzzle_midx = null;
var orm_puzzle_idx = null;
var orm_puzzle = null;
var orm_puzzle_next = null;
var orm_movelist = null;
var orm_movelist_idx = null;
var orm_candidate_move = null;

var orm_error = function(str) {
	var err = $(document.createElement('p'));
	err.addClass('alert alert-danger');
	err.text(str);
	$("nav#mainnav").after(err);
};

var orm_load_puzzle_manifest = function(done) {
	$.getJSON("./puzzles/manifest.js").always(function() {
		$("div#select-puzzleset > p.alert").remove();
	}).fail(function() {
		orm_error('Could not load the puzzle set manifest, make sure no extension is blocking XHR.');
	}).done(function(data) {
		orm_manifest = data;

		var ul = $("div#select-puzzleset > ul");
		var hash = location.hash.split('-', 3);
		for(var i in data) {
			var pset = data[i];
			var li = $(document.createElement('li'));
			li.addClass('row');

			var h = $(document.createElement('h3'));
			h.text(pset.name);
			h.addClass('col');
			li.append(h);

			var p = $(document.createElement('p'));
			p.text(pset.desc + " " + pset.count + " puzzles.");
			p.addClass('col-6');
			li.append(p);

			var btn = $(document.createElement('button'));
			btn.text('Start training');
			btn.addClass('btn btn-primary col start-training');
			btn.data('idx', i);
			li.append(btn);

			ul.append(li);
		}

		ul.find("button.start-training").click(function() {
			var t = $(this);
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

var orm_load_puzzle_set = function(m_idx, always, fail, done) {
	var p = $(document.createElement('p'));
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

var orm_restore_tab = function() {
	$("div#intro").hide();
	var h = location.hash.split("-", 3);
	switch(h[0]) {
	case "#list":
		$("div#select-puzzleset").show();
		return;

	case "#puzzle":
		/* XXX */
		if(h.length !== 3) break;
		var manifest_idx = null;
		for(var i in orm_manifest) {
			if(orm_manifest[i].id === h[1]) {
				manifest_idx = i;
				break;
			}
		}
		if(manifest_idx === null) break;
		orm_load_puzzle_set(manifest_idx, null, function() {
			$("div#intro").show();
		}, function() {
			var idx = parseInt(h[2]);
			orm_load_puzzle(idx);
			$("div#play-puzzle").show();
		});
		return;
	}

	$("div#intro").show();
};

var orm_init_board = function() {
	var b = $("div#board");
	var d;
	for(var f = 1; f <= 8; ++f) {
		for(var r = 1; r <= 8; ++r) {
			d = $(document.createElement('div'));
			d.addClass("back r" + r + " f" + f + " " + ((f + r) % 2 ? 'light' : 'dark'));
			d.data("ofile", f);
			d.data("orank", r);
			b.append(d);
		}
	}
};

var orm_load_fen = function(fen, move, done) {
	var b = $("div#board");
	var p, r = 8, f = 1, cl;
	b.children("div.piece").remove();

	for(var i = 0; i < fen.length && fen[i] != " "; ++i) {
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
			f += parseInt(fen[i]);
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

	var side = fen.split(" ", 3)[1];
	if(side === "w") {
		$("div#board").removeClass("black").addClass("white");
	} else {
		$("div#board").removeClass("white").addClass("black");
	}
}

var orm_animate_move = function(startfen, movelan, endfen, start, done, delay) {
	if(startfen) orm_load_fen(startfen);

	var sf, sr, ef, er, cl;
	sf = movelan.charCodeAt(0) - "a".charCodeAt(0) + 1;
	sr = movelan.charCodeAt(1) - "1".charCodeAt(0) + 1;
	ef = movelan.charCodeAt(2) - "a".charCodeAt(0) + 1;
	er = movelan.charCodeAt(3) - "1".charCodeAt(0) + 1;
	/* XXX: promotion */

	var psrc = $("div#board > div.piece.f" + sf + ".r" + sr);
	var pdest = $("div#board > div.piece.f" + ef + ".r" + er);

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

var orm_push_move = function(startfen, movesan, movelan, endfen) {
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

	var btn = $(document.createElement('button'));
	btn.text(movesan);
	btn.data('idx', orm_movelist_idx);
	btn.attr("id", "ply" + orm_movelist_idx);
	btn.addClass('btn btn-sm btn-block btn-primary');

	var tb = $("table#movelist > tbody");
	var tr = tb.children('tr').last();
	var td;
	if(tr.length === 0 || tr.find('button').length === 2) {
		tr = $(document.createElement("tr"));
		tb.append(tr);

		var th = $(document.createElement("th"));
		th.text(Math.floor((orm_puzzle.ply + orm_movelist.length) / 2) + ".");
		tr.append(th);

		td = $(document.createElement("td"));
		tr.append(td);
		var td2 = $(document.createElement("td"));
		tr.append(td2);

		if((orm_puzzle.ply  + orm_movelist.length) % 2) {
			var dummy = $(document.createElement("button"));
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

var orm_load_puzzle = function(idx) {
	orm_puzzle_idx = idx;
	orm_movelist = orm_movelist_idx = null;
	var puz = orm_puzzle = orm_puzzle_set[idx];
	$("table#movelist > tbody").empty();
	$("a#puzzle-analysis").prop('href', 'https://lichess.org/analysis/' + orm_puzzle.reply.fen.replace(/\s/g, '_'));
	history.replaceState(null, null, "#puzzle-" + orm_manifest[orm_puzzle_midx].id + "-" + idx);

	$("div#board").toggleClass("flipped", !!(puz.ply % 2));
	orm_animate_move(puz.board, puz.reply.lan, puz.reply.fen, function() {
		orm_push_move(puz.board, puz.reply.san, puz.reply.lan, puz.reply.fen);
	});

	$("p#puzzle-prompt")
		.removeClass("text-success text-danger")
		.text(orm_manifest[orm_puzzle_midx].prompt.replace("{%side}", puz.ply % 2 ? "Black" : "White"));
	$("div#puzzle-actions-after").removeClass("visible");
	$("button#puzzle-abandon").show();
	$("button#puzzle-next").hide();
	$("nav#mainnav").removeClass("bg-success bg-danger");
	orm_puzzle_next = puz.next;
};

var orm_puzzle_over = function() {
	$("div#puzzle-actions-after").addClass('visible');
	$("button#puzzle-abandon").hide();
	$("button#puzzle-next").show();
	orm_puzzle_next = null;
};

var orm_puzzle_success = function() {
	$("p#puzzle-prompt").addClass("text-success").text("Puzzle completed successfully!");
	$("nav#mainnav").addClass("bg-success");
	orm_puzzle_over();
};

var orm_puzzle_fail = function() {
	var oldidx = orm_movelist_idx;
	var oldbtn = $("table#movelist button.btn-primary");
	while(orm_puzzle_next !== null) {
		for(var lan in orm_puzzle_next) {
			var puz = orm_puzzle_next[lan];
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

var orm_can_move_piece = function(p) {
	var b = $("div#board");
	if(p.hasClass("white") !== b.hasClass("white") || p.hasClass("black") !== b.hasClass("black")) return false;
	if(orm_puzzle !== null && orm_puzzle_next !== null && p.hasClass("black") !== !!(orm_puzzle.ply % 2)) return false;
	if(orm_puzzle_next !== null && orm_movelist_idx !== orm_movelist.length - 1) return false;
	return true;
};

var orm_do_user_move = function(lan, animate) {
	orm_candidate_move = null;

	/* XXX: promote */

	var sf, sr, tf, tr;
	sf = lan.charCodeAt(0) - "a".charCodeAt(0) + 1;
	sr = lan.charCodeAt(1) - "1".charCodeAt(0) + 1;
	tf = lan.charCodeAt(2) - "a".charCodeAt(0) + 1;
	tr = lan.charCodeAt(3) - "1".charCodeAt(0) + 1;

	if(sf < 1 || sf > 8 || sr < 1 || sr > 8 || tf < 1 || tf > 8 || tr < 1 || tr > 8) return false;
	if(sf === tf && sr === tr) return false;

	var b = $("div#board");
	b.toggleClass("white black");

	var after = function() {
		if(orm_puzzle_next === null) return true;
		if(lan in orm_puzzle_next) {
			var puz = orm_puzzle_next[lan];
			orm_animate_move(puz.move.fen, puz.reply.lan, puz.reply.fen, function() {
				orm_push_move(puz.move.fen, puz.reply.san, puz.reply.lan, puz.reply.fen);
			});
			orm_push_move(orm_movelist[orm_movelist_idx][3], puz.move.san, lan, puz.move.fen); /* XXX */
			orm_puzzle_next = puz.next;
			if(puz.next === null) {
				orm_puzzle_success();
			} else {
				$("p#puzzle-prompt").text("Good move! Keep going…");
			}
		} else {
			orm_puzzle_fail();
		}
	};

	if(animate) {
		orm_animate_move(null, lan, null, null, after, 0);
	} else {
		b.children("div.piece.r" + tr + ".f" + tf).remove();

		var p = b.children("div.piece.r" + sr + ".f" + sf);
		p.removeClass("r" + sr + " f" + sf);
		p.addClass("r" + tr + " f" + tf);
		p.data("orank", tr).data("ofile", tf);

		after();
	}

	return true;
};

$(function() {
	$("p#enable-js").remove();

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
		var b = $(this);
		orm_movelist_idx = b.data("idx");
		var m = orm_movelist[orm_movelist_idx];
		$("table#movelist button.btn-primary").removeClass("btn-primary").addClass("btn-light");
		b.removeClass("btn-light").addClass("btn-primary");
		orm_animate_move(m[0], m[2], m[3], null, null, 100);
	});

	/* XXX not touch-friendly */
	$("div#board").on("mousedown", "> div.piece", function(e) {
		var p = $(this);
		if(!orm_can_move_piece(p)) return;

		var bg = $("div#board > div.back.r" + p.data("orank") + ".f" + p.data("ofile"));
		bg.addClass("drag-source");
		p.data("origx", e.pageX);
		p.data("origy", e.pageY);
		p.data("origtop", p.position().top);
		p.data("origleft", p.position().left);
		p.addClass("dragging");
	}).on("mousemove", function(e) {
		var p = $("div#board > div.piece.dragging");
		if(!p) return;

		p.addClass("dragged");
		p.css("left", p.data("origleft") + e.pageX - p.data("origx"));
		p.css("top", p.data("origtop") + e.pageY - p.data("origy"));
	}).on("mouseup", "> div.piece.dragging", function(e) {
		var p = $(this);
		var b = $("div#board"), pos = p.position();
		var file = 1 + Math.round(8 * pos.left / b.width());
		var rank = 9 - Math.round(1 + 8 * pos.top / b.height());

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
		$("div#board > div.drag-source").removeClass("drag-source");
	}).on("click", "> div", function() {
		var p = $(this);
		if(p.hasClass("dragged")) {
			p.removeClass("dragged");
			return;
		}
		if(orm_candidate_move === null && (p.hasClass("back") || (p.hasClass("piece") && !orm_can_move_piece(p)))) return;

		if(orm_candidate_move === null) {
			var bg = $("div#board > div.back.r" + p.data("orank") + ".f" + p.data("ofile"));
			bg.addClass("drag-source");
			orm_candidate_move = String.fromCharCode(
				"a".charCodeAt(0) + p.data("ofile") - 1,
				"1".charCodeAt(0) + p.data("orank") - 1,
			);
			p.addClass("moving");
		} else {
			$("div#board > div.drag-source").removeClass("drag-source");

			var tgt = String.fromCharCode(
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
