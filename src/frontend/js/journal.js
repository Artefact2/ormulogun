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

const orm_journal_push = function(payload) {
	let j = orm_state_get("journal", []);
	let je = [ Date.now(), payload ];

	j.unshift(je);

	let maxl = parseInt(orm_pref("journal_max_length"), 10);
	let overflow = j.length - maxl;
	for(let i = 0; i < overflow; ++i) {
		j.pop();
	}

	orm_state_set("journal", j);
	orm_prepend_journal_entry(je);
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

	d.children('h2, ul').remove();

	let j = orm_state_get('journal', []);
	let jl = j.length;
	for(let i = jl - 1; i >= 0; --i) orm_prepend_journal_entry(j[i]);

	if(d.children('ul').length === 0) {
		d.append($(document.createElement('div')).append(
			$(document.createElement('p')).addClass('alert alert-secondary')
				.text('No recorded activity yet. Come back after solving a few puzzles.')
		));
	}
};

const orm_prepend_journal_entry = function(je) {
	let ul = $("div#journal > ul");
	if(ul.length === 0) {
		ul = $(document.createElement('ul'));
		$('div#journal').append(ul);
	}

	let li = $(document.createElement('li'));
	let btn = $(document.createElement('button'));
	let pset = null;

	btn.addClass('btn btn-sm disabled').prop('disabled', true);
	for(let i in orm_manifest) { /* XXX: make orm_manifest an object? */
		if(orm_manifest[i].id === je[1][0]) {
			btn.removeClass('disabled').prop('disabled', false);
			btn.data('midx', i);
			pset = orm_manifest[i].id;
			break;
		}
	}

	btn.data('puzidx', je[1][2]);
	btn.data('date', je[0]);
	btn.text('#' + je[1][2].toString());
	btn.addClass(je[1][1] === 0 ? 'btn-danger' : 'btn-success');

	if(pset === null) {
		btn.prop('title', 'Unknown puzzle set: ' + je[1][0]);
	} else {
		btn.prop('title', '[' + pset.name + '] ' + (new Date(je[0])).toString());
	}

	btn.prop('type', 'button');
	li.append(btn);

	ul.prepend(li);
};

orm_when_puzzle_manifest_ready.push(function() {
	orm_generate_journal_page();

	$("div#journal").on('click', '> ul > li > button', function() {
		/* XXX: refactor with load.js */
		let b = $(this);
		orm_load_puzzle_set(b.data('midx'), null, null, function() {
			orm_load_tab("board", true, function() {
				orm_load_puzzle(b.data('puzidx'));
			});
		});
	});
});
