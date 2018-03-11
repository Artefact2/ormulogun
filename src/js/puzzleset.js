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

let orm_puzzle_sets = {};
let orm_puzzle_set = null;
let orm_puzzle_midx = null;
let orm_temp_filter = false;

const orm_load_puzzle_set = function(div, always, fail, done) {
	let alert = $(document.createElement('p'));
	let m_idx = div.data('midx');
	let btn = div.find('button');
	div.children('h2').first().after(alert);
	alert.addClass("alert alert-primary").text("Loading puzzle set…");
	btn.addClass('disabled').prop('disabled', true);

	$.getJSON("./puzzles/" + orm_manifest[m_idx].src.replace(/\.json$/, '.js')).always(function() {
		btn.removeClass('disabled').prop('disabled', false);
		if(always) always();
	}).fail(function() {
		alert.toggleClass('alert-primary alert-danger');
		alert.text("Could not load the puzzle set : " + orm_manifest[m_idx].id + ".");
		if(fail) fail();
	}).done(function(data) {
		orm_puzzle_sets[m_idx] = data;
		orm_puzzle_set = data;
		orm_puzzle_midx = m_idx;
		if(done) done();
		alert.remove();
		div.find('button.load-puzzleset').toggleClass('btn-primary btn-success load-puzzleset play-puzzleset')
			.children('span').first().text('Start training');
		div.find('button.reload-puzzleset').show();
		/* XXX: regen tags, counts and progress info here */
	});
};

const orm_tag_priority = function(t) {
	t = t.match(/^([^(]+)( \([^)]+\))?$/)[1];

	/* General puzzle categories, usually mutually exclusive */
	if(t.match(/^(Checkmate|Draw|Winning position|Drawing position|Material (gain|loss)|Trade|Quiet)$/)) return 1;

	/* Depth stuff, very spammy as there's lots of them */
	if(t.match(/^(Depth|Min depth|Max depth) /)) return 3;

	/* Tactical motifs */
	if(t.match(/^(Pin|Fork|Skewer|Discovered (attack|check)|Double check|Capturing defender|(Overloaded|Trapped) piece)$/)) return 0;

	/* Fallback */
	return 2;
};

const orm_format_integer = function(n) {
	if(n < 0) return '-' + orm_format_integer(-n);
	if(n < 1000) return n.toString();

	let m = n % 1000;
	return orm_format_integer((n - m) / 1000) + ',' + (m < 10 ? ('00' + m) : (m < 100 ? '0' + m : m));
};

const orm_clear_temp_filter = function() {
	if(orm_temp_filter === false) return;
	orm_tag_whitelist.pop();
	orm_temp_filter = false;
};

orm_when_puzzle_manifest_ready.push(function() {
	let md = $("div#puzzlesets");
	let gen_set_div = function(i, ps) {
		md.append(
			$(document.createElement('div'))
				.data('midx', i)
				.addClass('puzzle-set mb-4')
				.prop('id', 'puzzle-set-' + i)
				.append(
					$(document.createElement('h2'))
						.addClass('d-flex justify-content-between border-bottom pb-1 border-dark')
						.append(
							$(document.createElement('span')).text(ps.name),
							$(document.createElement('span')).append(
								$(document.createElement('button')).prop('type', 'button')
									.addClass('mr-1 btn btn-outline-secondary reload-puzzleset')
									.text('Reload')
									.hide(),
								$(document.createElement('button')).prop('type', 'button')
									.addClass('btn btn-primary load-puzzleset')
									.append(
										$(document.createElement('span')).text('Load puzzles'),
										' ',
										$(document.createElement('span')).addClass('badge badge-light').text(orm_format_integer(ps.count))
									)
							)
						),
					$(document.createElement('p')).addClass('puzzleset-desc').text(ps.desc)
				)
		);
	};

	for(let i in orm_manifest) {
		gen_set_div(i, orm_manifest[i]);
	}

	md.on('click', 'button.load-puzzleset, button.reload-puzzleset', function() {
		orm_load_puzzle_set($(this).closest('div.puzzle-set'));
	}).on('click', 'button.play-puzzleset', function() {
		let d = $(this).closest('div.puzzle-set');
		orm_puzzle_midx = d.data('midx');
		orm_puzzle_set = orm_puzzle_sets[orm_puzzle_midx];
		orm_clear_temp_filter();
		orm_load_tab("board", true, function() {
			orm_load_next_puzzle();
		});
	});
	/* XXX: auto-load last played set from history */
});

/*orm_when_puzzle_manifest_ready.push*/(function() {
	let ul = $("div#puzzlesets > ul");
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
		btn.prop('type', 'button');
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

			let m = sortedtags[j].match(/^[^(]+\s\(([^)]+)\)$/);
			let l = tli.children().last();
			if(m && l.length > 0) {
				/* Add sub tag as dropdown */
				if(l.is("button")) {
					let div = $(document.createElement('div'));
					div.addClass('btn-group');
					div.append(l);
					l.removeClass('mr-1 mb-1');
					div.addClass('mr-1 mb-1');
					div.append($(document.createElement('button'))
							   .addClass(l.attr('class') + ' dropdown-toggle dropdown-toggle-split')
							   .prop('type', 'button')
							   .attr('data-toggle', 'dropdown'));
					div.append($(document.createElement('div'))
							   .addClass('dropdown-menu'));
					tli.append(div);
					l = div;
				}
				l.children("div.dropdown-menu").append(
					$(document.createElement('a'))
						.addClass('dropdown-item')
						.data('tag', sortedtags[j])
						.data('idx', i)
						.prop('href', '#')
						.text(m[1] + ' ').append(
							$(document.createElement('span'))
								.addClass('badge badge-light')
								.text(orm_format_integer(pset.tags[sortedtags[j]]))
						)
				);
				continue;
			}

			let btn = $(document.createElement('button'));
			let span = $(document.createElement('span'));
			btn.prop('type', 'button');
			btn.addClass('btn btn-sm mr-1 mb-1');
			btn.addClass('tag-prio-' + lastprio);
			btn.text(sortedtags[j] + ' ');
			btn.data('tag', sortedtags[j]).data('idx', i);
			btn.prop('type', 'button');
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

	ul.on('click', 'button, a.dropdown-item', function(e) {
		let t = $(this);
		if(t.hasClass('dropdown-toggle')) return;
		if(t.is('a')) e.preventDefault();

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
				orm_load_next_puzzle();
			});
		});
	});
});
