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

const orm_load_puzzle_manifest = function(done) {
	let alert = $(document.createElement('p'));
	$("div#puzzlesets > h1").after(alert);
	alert.addClass('alert alert-primary');
	alert.text('Loading the puzzle set manifest…');

	$.getJSON("./puzzles/manifest.js").fail(function() {
		alert.toggleClass('alert-primary alert-danger');
		alert.text('Error while fetching the puzzle set manifest. Make sure no extension is blocking XHR.');
	}).done(function(data) {
		orm_manifest = data;
		done();
		alert.remove();
	});
};

const orm_restore_tab = function() {
	$("div#intro").hide();
	let h = location.hash.split("-", 3);
	switch(h[0]) {
	case "#puzzle":
		/* XXX */
		if(h.length !== 3) break;
		$("div#puzzlesets").show();
		orm_when_puzzle_manifest_ready.push(function() {
			let manifest_idx = null;
			for(let i in orm_manifest) {
				if(orm_manifest[i].id === h[1]) {
					manifest_idx = i;
					break;
				}
			}
			if(manifest_idx === null) return;
			orm_load_puzzle_set(manifest_idx, null, null, function() {
				orm_load_tab("board", false, function() {
					orm_load_puzzle(parseInt(h[2], 10));
				});
			});
		});
		return;

	case "#board":
		/* XXX: refactor with orm_load_tab() */
		orm_reset_main_board();
		break;
	}

	let d = $("body > div.tab" + h[0]);
	if(d.length === 1) {
		d.show();
	} else {
		$("div#intro").show();
	}
};

const orm_init_board = function(b) {
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
	if(b.data('fen')) {
		b.toggleClass('flipped', b.data('fen').split(' ', 3)[1] === 'b');
		orm_load_fen(b.data('fen'), b);
	}
};

orm_when_ready.push(function() {
	/* Important: this may push stuff into orm_load_puzzle_manifest,
	 * so it has to come before */
	orm_restore_tab();

	orm_load_puzzle_manifest(function() {
		for(let i in orm_when_puzzle_manifest_ready) {
			orm_when_puzzle_manifest_ready[i]();
		}
	});

	$("div.board").each(function() { orm_init_board($(this)); });
});
