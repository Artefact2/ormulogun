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

const orm_load_puzzle_set = function(m_idx, always, fail, done) {
	let alert = $(document.createElement('p'));
	let div = $('div#puzzlesets > div#puzzle-set-' + m_idx);
	let btn = div.find('button');
	div.children('h4').first().after(alert);
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
		setTimeout(function() {
			orm_regen_puzzleset_counts(div);
			orm_generate_tag_prefs();
		}, 1);
	});
};

const orm_tag_priority = function(t) {
	t = t.match(/^([^(]+)( \([^)]+\))?$/)[1];

	/* General puzzle categories, usually mutually exclusive */
	if(t.match(/^(Checkmate|Draw|Winning position|Drawing position|Material (gain|loss)|Trade|Quiet)$/)) return 1;

	/* Tactical motifs */
	if(t.match(/^(Pin|Fork|Skewer|Discovered (attack|check)|Double check|Undermining|(Overloaded|Trapped) piece)|Deflection$/)) return 0;

	/* Fallback */
	return 2;
};

const orm_format_integer = function(n) {
	if(n < 0) return '-' + orm_format_integer(-n);
	if(n < 1000) return n.toString();

	let m = n % 1000;
	return orm_format_integer((n - m) / 1000) + ',' + (m < 10 ? ('00' + m) : (m < 100 ? '0' + m : m));
};

const orm_regen_puzzleset_counts = function(div) {
	let total = 0;
	let tags = {};
	let tagnames = [];
	let midx = div.data('midx');
	let ps = orm_puzzle_sets[midx];

	for(let i in ps) {
		if(orm_puzzle_filtered(ps[i])) continue;
		++total;

		for(let j in ps[i][2]) {
			let tag = ps[i][2][j];
			if(!(tag in tags)) {
				tags[tag] = 0;
				tagnames.push(tag);
			}
			++tags[tag];
		}
	}

	/* Sort by name, then by tag priority */
	tagnames.sort().sort(function(a, b) {
		return orm_tag_priority(a) - orm_tag_priority(b);
	});

	let ul = div.children('ul.puzzleset-tags').empty(), li;
	let lastprio = null;
	for(let i in tagnames) {
		let tn = tagnames[i];
		let tc = tags[tn];
		let prio = orm_tag_priority(tn);
		if(prio !== lastprio) {
			ul.append(li = $(document.createElement('li')));
			lastprio = prio;
		}

		let m = tn.match(/^[^(]+\s\(([^)]+)\)$/);
		let l = li.children().last();
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
				li.append(div);
				l = div;
			}
			l.children("div.dropdown-menu").append(
				$(document.createElement('a'))
					.addClass('tag-filter dropdown-item')
					.data('tag', tn)
					.prop('href', '#')
					.text(m[1] + ' ').append(
						$(document.createElement('span'))
							.addClass('badge badge-light')
							.text(orm_format_integer(tc))
					)
			);
			continue;
		}

		let btn = $(document.createElement('button'));
		let span = $(document.createElement('span'));
		btn.prop('type', 'button');
		btn.addClass('btn btn-sm mr-1 mb-1');
		btn.addClass('tag-filter tag-prio-' + lastprio);
		btn.text(tn + ' ');
		btn.data('tag', tn);
		btn.prop('type', 'button');
		span.addClass('badge badge-light');
		span.text(orm_format_integer(tc));
		btn.append(span);
		li.append(btn);
	}

	let b = orm_get_leitner_boxes(orm_manifest[midx].id);
	let bcounts = {};
	let boxes = [];
	let btotal = 0;
	for(let puzid in b) {
		if(orm_puzzle_filtered(ps[puzid])) continue;
		let box = b[puzid][0];
		if(!(box in bcounts)) {
			bcounts[box] = 0;
			boxes.push(box);
		}
		++bcounts[box];
		++btotal;
	}
	boxes.sort();

	div.find('div.puzzle-set-completion > span').text(
		'Completion rate: ' + (100.0 * btotal / total).toFixed(3) + '% ('
			+ orm_format_integer(btotal) + ' / ' + orm_format_integer(total) + ')'
	);
	div.find('div.puzzle-set-completion > div.progress').empty().append(
		$(document.createElement('div')).addClass('progress-bar bg-primary')
			.css('width', (100.0 * btotal / total).toString() + '%')
	);

	let pb = div.find('div.puzzle-set-mastery > div.progress').empty();
	let mb = parseInt(orm_pref('leitner_mastery_threshold'));
	let mtotal = 0.0;
	for(let i in boxes) {
		pb.prepend(
			$(document.createElement('div')).addClass('progress-bar bg-success')
				.css('width', (100.0 * bcounts[boxes[i]] / btotal).toString() + '%')
				.css('opacity', Math.min(boxes[i], mb) / mb)
		);
		if(boxes[i] >= mb) {
			mtotal += bcounts[boxes[i]];
		} else {
			mtotal += bcounts[boxes[i]] * (1.0 - Math.pow(.5, boxes[i]));
		}
	}
	div.find('div.puzzle-set-mastery > span').text(
		'Mastery rate: ' + (btotal > 0 ? 100.0 * mtotal / btotal : 0).toFixed(4) + '%'
	);

	let frate = 0.0;
	div.find('div.puzzle-set-filtered > span').text(
		'Filter rate: ' + (frate = 100.0 * (1.0 - total / ps.length)).toFixed(1) + '% ('
			+ orm_format_integer(ps.length - total) + ' / ' + orm_format_integer(ps.length) + ')'
	);
	div.find('div.puzzle-set-filtered > div.progress').empty().append(
		$(document.createElement('div')).addClass('progress-bar bg-secondary')
			.css('width', (100.0 - frate).toString() + '%')
	);

	div.find('button.play-puzzleset').children('span.badge').text(orm_format_integer(total));
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
					$(document.createElement('h4'))
						.addClass('d-flex justify-content-between align-items-end')
						.append(
							$(document.createElement('span')).text(ps.name),
							$(document.createElement('span')).append(
								$(document.createElement('button')).prop('type', 'button')
									.addClass('mr-1 btn btn-sm btn-outline-secondary reload-puzzleset')
									.text('Refresh stats')
									.hide(),
								$(document.createElement('button')).prop('type', 'button')
									.addClass('btn btn-sm btn-primary load-puzzleset')
									.append(
										$(document.createElement('span')).text('Load puzzles'),
										' ',
										$(document.createElement('span')).addClass('badge badge-light').text(orm_format_integer(ps.count))
									)
							)
						),
					$(document.createElement('div')).addClass('row ml-0 mr-0 mb-1 puzzle-set-info').append(
						$(document.createElement('div')).addClass('col-lg-4 p-0 puzzle-set-completion').append(
							$(document.createElement('div')).addClass('progress'), $(document.createElement('span'))
						),
						$(document.createElement('div')).addClass('col-lg-4 p-0 puzzle-set-mastery').append(
							$(document.createElement('div')).addClass('progress'), $(document.createElement('span'))
						),
						$(document.createElement('div')).addClass('col-lg-4 p-0 puzzle-set-filtered').append(
							$(document.createElement('div')).addClass('progress'), $(document.createElement('span'))
						)
					),
					$(document.createElement('ul')).addClass('puzzleset-tags'),
					$(document.createElement('div')).addClass('puzzleset-desc').html(ps.desc)
				)
		);
	};

	for(let i in orm_manifest) {
		gen_set_div(i, orm_manifest[i]);
	}

	md.on('click', 'button.load-puzzleset', function() {
		orm_load_puzzle_set($(this).closest('div.puzzle-set').data('midx'));
		$(this).blur();
	}).on('click', 'button.reload-puzzleset', function() {
		let t = $(this);
		setTimeout(function() {
			orm_regen_puzzleset_counts(t.closest('div.puzzle-set'));
		}, 1);
		t.blur();
	}).on('click', 'button.play-puzzleset, .tag-filter', function() {
		let t = $(this);
		t.blur();
		if(t.hasClass('dropdown-toggle')) return;
		let d = t.closest('div.puzzle-set');
		orm_puzzle_midx = d.data('midx');
		orm_puzzle_set = orm_puzzle_sets[orm_puzzle_midx];
		if(orm_temp_filter !== false) {
			orm_tag_whitelist.pop();
			orm_temp_filter = false;
		}
		if(t.hasClass('tag-filter')) {
			orm_tag_whitelist.push(t.data('tag'));
			orm_temp_filter = true;
		}
		orm_load_tab("board", true, function() {
			orm_load_next_puzzle();
		});
	});

	if(!location.hash.match(/^#puzzle-/)) {
		/* XXX, for obvious reasons */
		md.find("button.load-puzzleset").first().click();
	}
});
