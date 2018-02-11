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

const orm_load_puzzle_manifest = function(done) {
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
			p.append($(document.createElement('br')));
			for(let t in pset.tags) {
				let btn = $(document.createElement('button'));
				let span = $(document.createElement('span'));
				btn.addClass('btn btn-sm btn-secondary mr-1');
				btn.text(t + ' ');
				span.addClass('badge badge-light');
				span.text(pset.tags[t]);
				btn.append(span);
				p.append(btn);
			}
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

const orm_load_puzzle_set = function(m_idx, always, fail, done) {
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

const orm_restore_tab = function() {
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

const orm_init_board = function() {
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

orm_when_ready.push(function() {
	orm_load_puzzle_manifest(function() {
		orm_init_board();
		orm_restore_tab();
	});
});
