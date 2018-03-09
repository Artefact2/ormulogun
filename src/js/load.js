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
let orm_temp_filter = false;

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

const orm_load_puzzle_set = function(m_idx, always, fail, done) {
	let alert = $(document.createElement('p'));
	$("div#puzzlesets > h1").after(alert);
	alert.addClass("alert alert-primary").text("Loading puzzle set…");

	$.getJSON("./puzzles/" + orm_manifest[m_idx].src.replace(/\.json$/, '.js')).always(function() {
		if(always) always();
	}).fail(function() {
		alert.toggleClass('alert-primary alert-danger');
		alert.text("Could not load the puzzle set : " + orm_manifest[m_idx].id + ".");
		if(fail) fail();
	}).done(function(data) {
		orm_puzzle_set = data;
		orm_puzzle_midx = m_idx;
		if(done) done();
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
				let idx = parseInt(h[2], 10);
				orm_load_tab("board", false, function() {
					history.replaceState(null, null, h.join('-')); /* XXX */
					orm_load_puzzle(idx);
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

const orm_tag_priority = function(t) {
	t = t.match(/^([^(]+)( \([^)]+\))?$/)[1];

	/* General puzzle categories, usually mutually exclusive */
	if(t.match(/^(Checkmate|Winning position|Drawing position|Draw|Material gain|Trade|Quiet)$/)) return 1;

	/* Depth stuff, very spammy as there's lots of them */
	if(t.match(/^(Depth|Min depth|Max depth) /)) return 3;

	/* Tactical motifs */
	if(t.match(/^(Pin|Fork|Skewer|Discovered check|Double check|Capturing defender)$/)) return 0;

	/* Fallback */
	return 2;
};

let orm_format_integer = function(n) {
	if(n < 0) return '-' + orm_format_integer(-n);
	if(n < 1000) return n.toString();

	let m = n % 1000;
	return orm_format_integer((n - m) / 1000) + ',' + (m < 10 ? ('00' + m) : (m < 100 ? '0' + m : m));
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

orm_when_puzzle_manifest_ready.push(function() {
	let ul = $("div#puzzlesets > ul");
	let hash = location.hash.split('-', 3);
	for(let i in orm_manifest) {
		let pset = orm_manifest[i];
		let li = $(document.createElement('li'));
		li.addClass('mb-4');

		let h = $(document.createElement('h2'));
		let span = $(document.createElement('span'));
		let btn = $(document.createElement('button'));
		let badge = $(document.createElement('span'));
		h.addClass('d-flex justify-content-between border-bottom pb-1 border-dark');
		span.text(pset.name);
		btn.text('Start training ');
		btn.addClass('btn btn-primary');
		btn.data('idx', i).data('tag', null);
		badge.addClass('badge badge-light');
		badge.text(orm_format_integer(pset.count));
		btn.append(badge);
		h.append(span, btn);
		li.append(h);

		let tul = $(document.createElement('ul')), lastprio = null, tli = null;
		let sortedtags = [];
		for(let t in pset.tags) sortedtags.push(t); /* XXX: .keys() ? */
		sortedtags.sort(function(a, b) {
			return orm_tag_priority(a) - orm_tag_priority(b);
		});
		for(let j in sortedtags) {
			let prio = orm_tag_priority(sortedtags[j]);
			if(prio !== lastprio) {
				tul.append(tli = $(document.createElement('li')));
				lastprio = prio;
			}

			let btn = $(document.createElement('button'));
			let span = $(document.createElement('span'));
			btn.addClass('btn btn-sm mr-1 mb-1');
			btn.addClass('tag-prio-' + lastprio);
			btn.text(sortedtags[j] + ' ');
			btn.data('tag', sortedtags[j]).data('idx', i);
			span.addClass('badge badge-light');
			span.text(orm_format_integer(pset.tags[sortedtags[j]]));
			btn.append(span);
			tli.append(btn);
		}
		tul.addClass('m-0 p-0');
		li.append(tul);

		let p = $(document.createElement('p'));
		p.text(pset.desc);
		li.append(p);

		ul.append(li);
	}

	ul.find("button").click(function() {
		let t = $(this);
		t.prop('disabled', 'disabled').addClass('disabled');
		orm_load_puzzle_set(t.data('idx'), null, null, function() {
			if(orm_temp_filter === true) {
				orm_temp_filter = false;
				orm_tag_whitelist.pop();
			}
			if(t.data('tag') !== null) {
				orm_temp_filter = true;
				orm_tag_whitelist.push(t.data('tag'));
			}
			orm_load_tab("board", true, function() {
				t.prop('disabled', false).removeClass('disabled');
				orm_puzzle_idx = null;
				orm_load_next_puzzle();
			});
		});
	});
});
