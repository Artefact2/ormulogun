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

let orm_puzzle = null;
let orm_puzzle_next = null;

const orm_load_puzzle = function(idx) {
	orm_puzzle_idx = idx;
	let puz = orm_puzzle = orm_puzzle_set[idx];

	$("table#movelist > tbody").empty();
	history.replaceState(null, null, "#puzzle-" + orm_manifest[orm_puzzle_midx].id + "-" + idx);

	puz.side = puz[0].split(' ', 3)[1] === 'b';
	$("div#board").toggleClass('flipped', !puz.side);

	orm_movehist_reset();
	gumble_load_fen(puz[0]);
	orm_load_fen(puz[0]);
	setTimeout(function() {
		orm_do_legal_move(puz[1][0], true, function() {
			$("a#puzzle-analysis").prop('href', 'https://lichess.org/analysis/' + gumble_save_fen().replace(/\s/g, '_'));
			$("ul#movehist li.new").addClass('puzzle-reply');
		});
	}, 500);

	$("p#puzzle-prompt")
		.empty()
		.removeClass("text-success text-danger")
		.text("Find the best move for " + (puz.side ? 'White' : 'Black') + ".");
	$("div#puzzle-actions-after").removeClass("visible");
	$("button#puzzle-abandon").show();
	$("button#puzzle-next").hide();
	$("nav#mainnav").removeClass("bg-success bg-danger");
	orm_puzzle_next = puz[1][1];
};

const orm_puzzle_over = function() {
	$("div#puzzle-actions-after").addClass('visible');
	$("button#puzzle-abandon").hide();
	$("button#puzzle-next").show();

	let prompt = $("p#puzzle-prompt");
	prompt.append($(document.createElement('br')));
	for(let i in orm_puzzle[2]) {
		let span = $(document.createElement('span'));
		span.addClass('d-inline-block mr-1 badge badge-secondary');
		span.text(orm_puzzle[2][i]);
		prompt.append(span);
	}

	orm_puzzle_next = null;
};

const orm_puzzle_success = function() {
	$("ul#movehist li.new").addClass("good-move");
	orm_movehist_make_active(orm_movehist_current());
	$("p#puzzle-prompt").addClass("text-success").text("Puzzle completed successfully!");
	$("nav#mainnav").addClass("bg-success");
	orm_puzzle_over();
};

const orm_puzzle_fail = function() {
	let cur = $("ul#movehist li.new > button");
	if(!cur.parent().hasClass('puzzle-reply')) {
		cur.parent().addClass('bad-move');
	}

	/* XXX: set as main variation */
	var prev = $("ul#movehist li.puzzle-reply").last();
	gumble_load_fen(prev.data('fen'));
	gumble_play_legal_lan(prev.data('lan'));
	orm_movehist_make_active(prev);
	while(orm_puzzle_next) {
		for(let lan in orm_puzzle_next) {
			let fen = gumble_save_fen();
			let san = gumble_lan_to_san(lan);
			gumble_play_legal_lan(lan);
			orm_movehist_push(fen, lan, san);
			orm_movehist_current().addClass('good-move');
			if(orm_puzzle_next[lan] === 0) {
				orm_puzzle_next = 0;
				break;
			}
			fen = gumble_save_fen();
			san = gumble_lan_to_san(orm_puzzle_next[lan][0]);
			gumble_play_legal_lan(orm_puzzle_next[lan][0]);
			orm_movehist_push(fen, orm_puzzle_next[lan][0], san);
			orm_puzzle_next = orm_puzzle_next[lan][1];
			break;
		}
	}

	gumble_load_fen(cur.parent().data('fen'));
	gumble_play_legal_lan(cur.parent().data('lan'));
	orm_movehist_make_active(cur.parent());

	$("p#puzzle-prompt").addClass("text-danger").text("Puzzle failed.");
	$("nav#mainnav").addClass("bg-danger");
	orm_puzzle_over();
};

const orm_puzzle_try = function(lan) {
	if(!orm_puzzle_next) return;
	let current = orm_movehist_current();
	if(!current.hasClass('new')) return;

	if(!orm_movehist_in_rootline(orm_movehist_current())) {
		orm_puzzle_fail();
		return;
	}

	if(!(lan in orm_puzzle_next)) {
		orm_puzzle_fail();
		return;
	}

	if(orm_puzzle_next[lan] === 0) {
		orm_puzzle_success();
		return;
	}

	const puz = orm_puzzle_next[lan];
	orm_puzzle_next = puz[1];

	current.addClass('good-move');
	orm_movehist_make_active(current);
	$("p#puzzle-prompt").text("Good move! Keep going…");
	setTimeout(function() {
		orm_do_legal_move(puz[0], true, function() {
			$("ul#movehist li.new").addClass('puzzle-reply');
		});
	}, 500);
};

orm_when_ready.push(function() {
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
});
