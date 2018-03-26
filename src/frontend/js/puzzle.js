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
let orm_puzzle_idx = null;
let orm_puzzle_next = null;

const orm_load_puzzle = function(puz) {
	let b = orm_get_board();

	if(typeof(puz) === "number") {
		orm_puzzle_idx = puz;
		puz = orm_puzzle = orm_puzzle_set[puz];
		history.replaceState(null, null, "#puzzle-" + orm_manifest[orm_puzzle_midx].id + "-" + orm_puzzle_idx);
	} else {
		orm_puzzle_idx = null;
		orm_puzzle = puz;
	}

	$("table#movelist > tbody").empty();

	puz.side = puz[0].split(' ', 3)[1] === 'b';
	b.toggleClass('flipped', !puz.side);

	orm_movehist_reset();
	gumble_load_fen(puz[0]);
	orm_load_fen(puz[0], b);
	setTimeout(function() {
		orm_do_legal_move(puz[1][0], true, function() {
			$("ul#movehist li.new").addClass('puzzle-reply');
		}, undefined, undefined, b);
	}, orm_pref("board_move_delay"));

	let prompt = $("p#puzzle-prompt")
		.empty()
		.removeClass("alert-success alert-danger")
		.addClass('alert-secondary')
		.text("Find the best move for " + (puz.side ? 'White' : 'Black') + ".")
	;
	if(orm_puzzle_idx !== null) {
		prompt.toggleClass('practice-mode', orm_get_puzzle_cooldown(orm_manifest[orm_puzzle_midx].id, orm_puzzle_idx) > Date.now());
	}

	$("nav#mainnav").removeClass("bg-success bg-danger");
	$("div#puzzle-stuff, .puzzle-during").show();
	$(".puzzle-cheat, .puzzle-after").hide();
	$(".puzzle-disable").addClass('disabled').prop('disabled', true);
	orm_puzzle_next = puz[1][1];
	orm_uci_stopall();
	orm_book_close();
};

const orm_reset_main_board = function() {
	orm_puzzle = null;
	orm_puzzle_next = null;
	$("div#puzzle-stuff").hide();
	$(".puzzle-cheat").show();
	$(".puzzle-disable").removeClass('disabled').prop('disabled', false);
	orm_movehist_reset();
	orm_uci_stopall();
	orm_book_close();
	$("div#book-stuff > div").empty();
	orm_get_board()
		.data('candidate-move', null)
		.removeClass('game-over flipped')
		.children("div.back").removeClass('move-prev pv-move-source pv-move-target move-source move-target');
	Module._cch_init_board(gumble_board); /* XXX: refactor me */
	orm_load_fen(gumble_save_fen(), orm_get_board());
};

const orm_puzzle_filtered = function(puz) {
	let mind = parseInt(orm_pref("puzzle_depth_min"), 10);
	let maxd = parseInt(orm_pref("puzzle_depth_max"), 10);
	let mindepth = null, maxdepth = null, depth = null;

	for(let j in puz[2]) {
		let t = puz[2][j], m;
		if(m = t.match(/^(Min depth|Max depth|Depth) ([1-9][0-9]*)$/)) {
			if(m[1] === "Min depth") mindepth = parseInt(m[2], 10);
			else if(m[1] === "Max depth") maxdepth = parseInt(m[2], 10);
			else depth = parseInt(m[2], 10);
			continue;
		}

		/* XXX: probably slow and inefficient, maybe use bitmasks?
		 * but it doesn't work for >32 tags */
		if($.inArray(t, orm_tag_blacklist) !== -1) return true;
	}

	if(mindepth !== null && maxdepth !== null && depth === null) {
		if(mindepth < mind || maxdepth > maxd) return true;
	} else if(depth !== null && mindepth === null && maxdepth === null) {
		if(depth < mind || depth > maxd) return true;
	} else {
		orm_error("Puzzle #" + i + " has invalid depth tagging.");
	}

	for(let j in orm_tag_whitelist) {
		/* XXX */
		if($.inArray(orm_tag_whitelist[j], puz[2]) === -1) return true;
	}

	return false;
};

const orm_load_next_puzzle = function() {
	let npk = "np_" + orm_manifest[orm_puzzle_midx].id, np;
	if((np = orm_state_get(npk, null)) !== null) {
		if(!orm_puzzle_filtered(orm_puzzle_set[np])) {
			return orm_load_puzzle(np);
		}
	}

	let i = orm_find_candidate_puzzle(orm_manifest[orm_puzzle_midx].id, orm_puzzle_set);
	if(i !== false) {
		orm_state_set(npk, i);
		return orm_load_puzzle(i);
	}

	/* XXX */
	orm_error("No more puzzles to play, try broadening the filtering in the preferences or try another puzzle set.");
};

const orm_puzzle_over = function() {
	$(".puzzle-during").hide();
	$(".puzzle-cheat, .puzzle-after").show();
	$(".puzzle-disable").removeClass('disabled').prop('disabled', false);

	let prompt = $("p#puzzle-prompt");
	prompt.append($(document.createElement('br')));
	orm_puzzle[2].sort(function(a, b) {
		return orm_tag_priority(a) - orm_tag_priority(b);
	});
	for(let i in orm_puzzle[2]) {
		let span = $(document.createElement('span'));
		span.addClass('d-inline-block mr-1 badge text-dark tag-prio-' + orm_tag_priority(orm_puzzle[2][i]));
		span.text(orm_puzzle[2][i]);
		prompt.append(span);
	}

	orm_puzzle_next = null;

	if(orm_puzzle_idx !== null) {
		let npk = "np_" + orm_manifest[orm_puzzle_midx].id;
		if(orm_state_get(npk, null) === orm_puzzle_idx) {
			orm_state_unset(npk);
		}
	}

	/* This can be a long-ish operation, don't hang the main "thread" */
	setTimeout(function() {
		orm_movehist_merge_from_puzzle(orm_puzzle);
	}, 1);
};

const orm_puzzle_success = function() {
	$("ul#movehist li.new").addClass("good-move");
	$("p#puzzle-prompt").toggleClass("alert-secondary alert-success").text("Puzzle completed successfully!");
	$("nav#mainnav").addClass("bg-success");
	if(orm_puzzle_idx !== null) orm_commit_puzzle_win(orm_manifest[orm_puzzle_midx].id, orm_puzzle_idx);
	orm_puzzle_over();
};

const orm_puzzle_fail = function() {
	let cur = $("ul#movehist li.new > button");
	if(!cur.parent().hasClass('puzzle-reply')) {
		cur.parent().addClass('bad-move');
		if(orm_pref("uci_start_after_fail") === "1") {
			$("a#engine-analyse").click();
		}
	}
	$("p#puzzle-prompt").toggleClass("alert-secondary alert-danger").text("Puzzle failed.");
	$("nav#mainnav").addClass("bg-danger");
	if(orm_puzzle_idx !== null) orm_commit_puzzle_loss(orm_manifest[orm_puzzle_midx].id, orm_puzzle_idx);
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

	if(!Array.isArray(orm_puzzle_next[lan])) {
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
		}, undefined, undefined, orm_get_board());
	}, orm_pref("board_move_delay"));
};

orm_when_ready.push(function() {
	$("button#puzzle-retry").click(function() {
		orm_load_puzzle(orm_puzzle_idx === null ? orm_puzzle : orm_puzzle_idx);
	});

	$("button#puzzle-abandon").click(function() {
		orm_puzzle_fail();
	});

	$("button#puzzle-next").click(function() {
		orm_load_next_puzzle();
	});
});
