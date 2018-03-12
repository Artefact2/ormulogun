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

const orm_journal_push = function(setid, payload) {
	let j = orm_state_get("journal_" + setid, []);
	let je;

	j.push(je = [ Date.now(), payload ]);

	let maxl = parseInt(orm_pref("journal_max_length"), 10);
	let overflow = j.length - maxl;
	for(let i = 0; i < overflow; ++i) {
		j.shift();
	}

	orm_state_set("journal_" + setid, j);

	let d = $("div#journal > div").filter(function() { return $(this).data('setid') === setid; });
	if(d.length > 0) {
		orm_prepend_journal_entry(d.data('midx'), je, d.children('ul'));
		d.parent().children('h1').after(d);
	} else {
		/* XXX */
		orm_generate_journal_page();
	}
};

const orm_journal_merge = function(j1, j2) {
	let j = [];
	while(j1.length && j2.length) {
		if(j1[0][0] === j2[0][0]) {
			/* XXX: assume duplicate */
			j1.shift();
			j.push(j2.shift());
			continue;
		}
		j.push(j1[0][0] < j2[0][0] ? j1.shift() : j2.shift());
	}
	while(j1.length) j.push(j1.shift());
	while(j2.length) j.push(j2.shift());
	return j;
};

const orm_generate_journal_page = function() {
	let d = $("div#journal");
	d.children("div").remove();

	for(var i in orm_manifest) {
		let child = orm_generate_journal_section(i);
		if(child.length === 0) continue;
		d.append(child);
	}

	if(d.children("div").length === 0) {
		d.append($(document.createElement('div')).append(
			$(document.createElement('p')).addClass('alert alert-secondary')
				.text('No recorded activity yet. Come back after solving a few puzzles.')
		));
		return;
	}

	/* Sort by reverse chronological order */
	d.children("div").sort(function(a, b) {
		return $(b).data('date') - $(a).data('date');
	}).appendTo(d);
};

const orm_generate_journal_section = function(midx) {
	let pm = orm_manifest[midx];
	let journal = orm_state_get("journal_" + pm.id, []);
	if(journal.length === 0) return $();

	let d = $(document.createElement('div'));
	d.data('setid', pm.id);
	d.data('midx', midx);
	d.append($(document.createElement('h4')).text(pm.name));
	let ul = $(document.createElement('ul')); /* XXX: categorize by date perhaps? ie "today", "this week", etc */
	d.append(ul);
	let jl = journal.length;
	for(let i = 0; i < jl; ++i) {
		orm_prepend_journal_entry(midx, journal[i], ul);
	}
	d.data('date', journal[jl - 1][0]);
	return d;
};

const orm_prepend_journal_entry = function(midx, je, ul) {
	let li = $(document.createElement('li'));
	let btn = $(document.createElement('button'));
	btn.addClass('btn btn-sm');
	btn.data('midx', midx);
	btn.data('puzidx', je[1][1]);
	btn.data('date', je[0]);
	btn.text('#' + je[1][1].toString());
	btn.addClass(je[1][0] === 0 ? 'btn-danger' : 'btn-success');
	btn.prop('title', (new Date(je[0])).toString());
	btn.prop('type', 'button');
	li.append(btn);

	let pdate = ul.find('> li:first-child > button').data('date');
	if(pdate && je[0] - pdate > 1800000) { /* Make this a pref maybe? */
		ul.prepend($(document.createElement('li')).addClass('spacer'));
	}

	ul.prepend(li);
};

orm_when_puzzle_manifest_ready.push(function() {
	orm_generate_journal_page();

	$("div#journal").on('click', ' > div > ul > li > button', function() {
		/* XXX: refactor with load.js */
		let b = $(this);
		orm_load_puzzle_set(b.data('midx'), null, null, function() {
			orm_load_tab("board", true, function() {
				orm_load_puzzle(b.data('puzidx'));
			});
		});
	});
});
