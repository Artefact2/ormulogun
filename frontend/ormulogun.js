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

orm_manifest = null;
orm_puzzle_set = null;
orm_puzzle_midx = null;
orm_puzzle_idx = null;
orm_puzzle = null;
orm_puzzle_next = null;

orm_error = function(str) {
	var err = $(document.createElement('p'));
	err.addClass('alert alert-danger');
	err.text(str);
	$("nav#mainnav").after(err);
};

orm_load_puzzle_manifest = function(done) {
	$.getJSON("./puzzles/manifest.json").always(function() {
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

orm_load_puzzle_set = function(m_idx, always, fail, done) {
	var p = $(document.createElement('p'));
	p.addClass("alert alert-primary").text("Loading the puzzle set…");
	$("nav#mainnav").after(p);

	$.getJSON("./puzzles/" + orm_manifest[m_idx].src).always(function() {
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

orm_restore_tab = function() {
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

orm_init_board = function() {
	var b = $("div#board");
	var d;
	for(var f = 1; f <= 8; ++f) {
		for(var r = 1; r <= 8; ++r) {
			d = $(document.createElement('div'));
			d.addClass("back r" + r + " f" + f + " " + ((f + r) % 2 ? 'light' : 'dark'));
			b.append(d);
		}
	}
};

orm_load_fen = function(fen, move, done) {
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

orm_animate_move = function(startfen, movelan, endfen) {
	orm_load_fen(startfen);

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
		pdest.addClass('captured');
		setTimeout(function() {
			psrc.removeClass("moving");
			orm_load_fen(endfen);
		}, 500);
	}, 750);
};

orm_load_puzzle = function(idx) {
	orm_puzzle_idx = idx;
	var puz = orm_puzzle = orm_puzzle_set[idx];
	history.replaceState(null, null, "#puzzle-" + orm_manifest[orm_puzzle_midx].id + "-" + idx);

	$("div#board").toggleClass("flipped", !!(puz.ply % 2));
	orm_animate_move(puz.board, puz.reply.lan, puz.reply.fen);

	$("p#puzzle-prompt")
		.removeClass("text-success text-danger")
		.text(orm_manifest[orm_puzzle_midx].prompt.replace("{%side}", puz.ply % 2 ? "Black" : "White"));
	$("button#puzzle-retry").removeClass("visible");
	$("button#puzzle-abandon").show();
	$("button#puzzle-next").hide();
	$("nav#mainnav").removeClass("bg-success bg-danger");
	orm_puzzle_next = puz.next;
};

orm_puzzle_over = function() {
	$("button#puzzle-retry").addClass("visible");
	$("button#puzzle-abandon").hide();
	$("button#puzzle-next").show();
	orm_puzzle_next = null;
};

orm_puzzle_success = function() {
	$("p#puzzle-prompt").addClass("text-success").text("Puzzle completed successfully!");
	$("nav#mainnav").addClass("bg-success");
	orm_ouzzle_over();
};

orm_puzzle_fail = function() {
	$("p#puzzle-prompt").addClass("text-danger").text("Puzzle failed.");
	$("nav#mainnav").addClass("bg-danger");
	orm_puzzle_over();
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
		/* XXX */
		$("button#puzzle-abandon").hide();
		$("button#puzzle-next").show();
	});

	$("button#puzzle-next").click(function() {
		/* XXX */
		orm_load_puzzle(orm_puzzle_idx + 1);
	});

	/* XXX not touch-friendly */
	$("div#board").on("mousedown", "> div.piece", function(e) {
		var p = $(this);
		var b = $("div#board");
		if(p.hasClass("white") !== b.hasClass("white") || p.hasClass("black") !== b.hasClass("black")) return;
		if(orm_puzzle !== null && orm_puzzle_next !== null && p.hasClass("black") !== !!(orm_puzzle.ply % 2)) return;

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

		if(file >= 1 && file <= 8 && rank >= 1 && rank <= 8 && (p.data("ofile") !== file || p.data("orank") !== rank)) {
			/* XXX: promotion */
			var lan = String.fromCharCode(
				"a".charCodeAt(0) + p.data("ofile") - 1,
				"1".charCodeAt(0) + p.data("orank") - 1,
				"a".charCodeAt(0) + file - 1,
				"1".charCodeAt(0) + rank - 1
			);

			if(orm_puzzle_next !== null && lan in orm_puzzle_next) {
				var puz = orm_puzzle_next[lan];
				orm_animate_move(puz.move.fen, puz.reply.lan, puz.reply.fen);
				orm_puzzle_next = puz.next;
				if(puz.next === null) {
					orm_puzzle_success();
				} else {
					$("p#puzzle-prompt").text("Good move! Keep going…");
				}
			} else {
				if(orm_puzzle_next === null) {
					p.removeClass("r1 r2 r3 r4 r5 r6 r7 r8 f1 f2 f3 f4 f5 f6 f7 f8");
					b.children("div.piece.r" + rank + ".f" + file).addClass("captured");
					p.addClass("r" + rank + " f" + file);
					p.data("orank", rank);
					p.data("ofile", file);
					b.toggleClass("white black");
				} else {
					orm_puzzle_fail();
				}
			}
		}

		p.removeAttr('style');
		p.removeClass('dragging');
		$("div#board > div.drag-source").removeClass("drag-source");
	});

	orm_load_puzzle_manifest(function() {
		orm_init_board();
		orm_restore_tab();
	});
});
