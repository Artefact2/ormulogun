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
			btn.addClass('btn btn-primary col');
			btn.data('src', pset.src).data('id', pset.id);
			li.append(btn);

			ul.append(li);
		}

		done();
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
		if(h.length === 3) {
			var manifest_idx = null;
			for(var i in orm_manifest) {
				if(orm_manifest[i].id === h[1]) {
					manifest_idx = i;
					break;
				}
			}
			if(manifest_idx === null) break;

			var p = $(document.createElement('p'));
			p.addClass("alert alert-primary").text("Loading the puzzle set…");
			$("nav#mainnav").after(p);

			$.getJSON("./puzzles/" + orm_manifest[manifest_idx].src).always(function() {
				p.remove();
			}).fail(function() {
				orm_error("Could not load the puzzle set.");
			}).done(function(data) {
				console.log(data[parseInt(h[2])]);
				orm_load_puzzle(manifest_idx, data[parseInt(h[2])]);
				$("div#play-puzzle").show();
			});
		}
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

orm_load_fen = function(fen) {
	var b = $("div#board");
	var p, r = 8, f = 1, cl;
	b.children("div.black, div.white").remove();

	for(var i = 0; i < fen.length; ++i) {
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

		case 'p': cl = 'black pawn'; break;
		case 'n': cl = 'black knight'; break;
		case 'b': cl = 'black bishop'; break;
		case 'r': cl = 'black rook'; break;
		case 'q': cl = 'black queen'; break;
		case 'k': cl = 'black king'; break;

		case 'P': cl = 'white pawn'; break;
		case 'N': cl = 'white knight'; break;
		case 'B': cl = 'white bishop'; break;
		case 'R': cl = 'white rook'; break;
		case 'Q': cl = 'white queen'; break;
		case 'K': cl = 'white king'; break;

		case ' ': return;
		default: cl = false; break;
		}

		if(cl === false || r < 1 || f > 8) {
			console.log("error parsing fen", fen);
			return;
		}

		p = $(document.createElement('div'));
		p.addClass(cl + " f" + f + " r" + r);
		b.append(p);

		++f;
	}
};

orm_load_puzzle = function(m_idx, puz) {
	orm_load_fen(puz[3]);

	//$("div#board").toggleClass('flipped', puz[0] % 2);
	$("span#puzzle-prompt").text(orm_manifest[m_idx].prompt.replace("{%side}", puz[0] % 2 ? "Black" : "White"));
	$("button#puzzle-next").addClass('btn-danger').text('Give up');
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

	orm_load_puzzle_manifest(function() {
		orm_init_board();
		orm_restore_tab();
	});
});
