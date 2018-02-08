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
		orm_do_legal_move(puz[1], true, function() {
			$("a#puzzle-analysis").prop('href', 'https://lichess.org/analysis/' + gumble_save_fen().replace(/\s/g, '_'));
		});
	}, 500);

	$("p#puzzle-prompt")
		.removeClass("text-success text-danger")
		.text(orm_manifest[orm_puzzle_midx].prompt.replace("{%side}",  puz.side ? 'White' : 'Black'));
	$("div#puzzle-actions-after").removeClass("visible");
	$("button#puzzle-abandon").show();
	$("button#puzzle-next").hide();
	$("nav#mainnav").removeClass("bg-success bg-danger");
	orm_puzzle_next = puz[2];
};

const orm_puzzle_over = function() {
	$("div#puzzle-actions-after").addClass('visible');
	$("button#puzzle-abandon").hide();
	$("button#puzzle-next").show();
	orm_puzzle_next = null;
};

const orm_puzzle_success = function() {
	$("p#puzzle-prompt").addClass("text-success").text("Puzzle completed successfully!");
	$("nav#mainnav").addClass("bg-success");
	orm_puzzle_over();
};

const orm_puzzle_fail = function() {
	/* XXX: push remaining moves and set as main variation */
	$("p#puzzle-prompt").addClass("text-danger").text("Puzzle failed.");
	$("nav#mainnav").addClass("bg-danger");
	orm_puzzle_over();
};

const orm_puzzle_try = function(lan) {
	if(orm_puzzle_next === null) return;

	if(!(lan in orm_puzzle_next)) {
		orm_puzzle_fail();
		return;
	}

	if(orm_puzzle_next[lan] === null) {
		orm_puzzle_success();
		return;
	}

	const puz = orm_puzzle_next[lan];
	orm_puzzle_next = puz[1];

	$("p#puzzle-prompt").text("Good move! Keep going…");
	setTimeout(function() {
		orm_do_legal_move(puz[0], true);
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
