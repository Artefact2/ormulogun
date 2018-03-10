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

const ORM_PREFS_DEFAULTS = {
	"highlight_moves": "1",
	"highlight_check": "1",
	"highlight_prev_move": "1",
	"board_max_size": "0",
	"board_animation_speed": "500",
	"board_move_delay": "500",
	"custom_css": "",

	"puzzle_depth_min": "1",
	"puzzle_depth_max": "7",

	"uci_multipv": "5",
	"uci_hard_limiter": "depth 22",
	"uci_practice_limiter": "movetime 1000",

	"leitner_cooldown_initial": "43200000", /* 12 hours: 12*3600*1000 ms */
	"leitner_cooldown_increase_arithmetic": "86400000", /* 24 hours */
	"leitner_cooldown_increase_geometric": "1.5",
	"leitner_first_win_initial": "5",
};

let orm_prefs = null;
let orm_prefs_prev = null;

let orm_tag_whitelist = [];
let orm_tag_blacklist = [];

const orm_pref = function(k) {
	return (k in orm_prefs) ? orm_prefs[k] : ORM_PREFS_DEFAULTS[k];
};

const orm_prefs_combine2 = function(d1, d2) {
	d1.children('div.col-sm-9').addClass('col-lg-3 mb-3');
	d2.children('div.col-sm-9').addClass('col-lg-3 mb-3');
	return d1.append(d2.children()).toggleClass('mb-3 mb-0');
};

const orm_prefs_generic = function(k, lstr, element) {
	let d = $(document.createElement('div'));
	let label = $(document.createElement('label'));
	let override = $(document.createElement('input'));
	let olabel = $(document.createElement('label'));
	let group = $(document.createElement('div'));

	element.addClass('form-control save-pref');
	element.data('k', k);

	d.addClass('form-row form-group mb-3');
	group.addClass('input-group');

	label.text(lstr);
	label.attr('for', element.prop('id'));
	label.addClass('text-sm-right');

	override.prop('type', 'checkbox');
	override.prop('id', 'override_s_' + k);
	override.prop('title', 'Override the default value?');

	group.append($(document.createElement('div')).addClass('input-group-prepend').append(
		$(document.createElement('div')).addClass('input-group-text').append(
			override
		)
	), element);

	override.change(function() {
		if(override.prop('checked')) {
			element.prop('disabled', false).removeClass('disabled');

			if(typeof(element.data('prev-val')) !== "undefined") {
				element.val(element.data('prev-val'));
				element.removeData('prev-val');
			}
		} else {
			element.prop('disabled', true).addClass('disabled');

			if(element.val() !== ORM_PREFS_DEFAULTS[k]) {
				element.data('prev-val', element.val());
			}

			element.val(ORM_PREFS_DEFAULTS[k]);
		}
	});

	if(k in orm_prefs) {
		element.data('prev-val', orm_prefs[k]);
	} else if(k in orm_prefs_prev) {
		element.val(orm_prefs_prev[k]);
	} else {
		element.val(ORM_PREFS_DEFAULTS[k]);
	}
	override.prop('checked', k in orm_prefs).change();

	d.append(label.addClass('col-sm-3 col-form-label'));
	d.append($(document.createElement('div')).addClass('col-sm-9').append(group));
	return d;
};

const orm_prefs_boolean = function(k, lstr) {
	let select = $(document.createElement('select'));
	select.addClass('custom-select');
	select.prop('id', 's_' + k);
	select.append($(document.createElement('option')).prop('value', 1).text('yes'));
	select.append($(document.createElement('option')).prop('value', 0).text('no'));
	return orm_prefs_generic(k, lstr, select);
};

const orm_prefs_number = function(k, lstr) {
	let input = $(document.createElement('input'));
	input.prop('id', 's_' + k);
	input.prop('type', 'number');
	return orm_prefs_generic(k, lstr, input);
};

const orm_prefs_string = function(k, lstr) {
	let input = $(document.createElement('input'));
	input.prop('id', 's_' + k);
	input.prop('type', 'text');
	return orm_prefs_generic(k, lstr, input);
}

const orm_prefs_textarea = function(k, lstr) {
	let ta = $(document.createElement('textarea'));
	ta.prop('id', 's_' + k);
	return orm_prefs_generic(k, lstr, ta);
}

const orm_prefs_css_update = function() {
	let css = '';

	if("custom_css" in orm_prefs) css += orm_prefs.custom_css;
	if("highlight_moves" in orm_prefs && orm_prefs.highlight_moves === "0") {
		css += "div.board > div.back.move-target:before { content: unset; }";
	}
	if("highlight_check" in orm_prefs && orm_prefs.highlight_check === "0") {
		css += "div.board > div.piece.king.in-check:after { content: unset; }";
	}
	if("highlight_prev_move" in orm_prefs && orm_prefs.highlight_prev_move === "0") {
		css += "div.board > div.back.move-prev { border: unset; }";
	}
	if("board_max_size" in orm_prefs) {
		css += "div.board.board-main { max-width: " + orm_prefs.board_max_size + "px; }";
	}

	let spd = orm_pref("board_animation_speed");
	css += "div.board > div.piece.moving { transition-duration: " + spd + "ms; }";
	css += "div.board > div.piece.captured { transition-duration: " + (spd / 2) + "ms; transition-delay: " + (spd / 2) + "ms; }";

	let style = $("head > style#prefs");
	if(style.length === 0) {
		style = $(document.createElement('style'));
		style.prop('id', 'prefs');
		$('head').append(style);
	}
	style.text(css);
};

const orm_prefs_apply = function() {
	orm_prefs_css_update();

	orm_temp_filter = false;
	orm_tag_whitelist = [];
	orm_tag_blacklist = [];
	for(let k in orm_prefs) {
		if(orm_prefs[k] !== "0" && orm_prefs[k] !== "2") continue;
		if(!k.match(/^puzzletag:/)) continue;
		let t = k.split(':', 2)[1];
		if(orm_prefs[k] === "0") orm_tag_blacklist.push(t);
		else orm_tag_whitelist.push(t);
	}

	let ul = $("div#analysis-stuff > ul").empty();
	let nlines = orm_pref('uci_multipv');
	for(let i = 0; i < nlines; ++i) {
		ul.append($(document.createElement('li')).append(
			document.createElement('strong'),
			document.createElement('span')
		));
	}
};

orm_when_ready.push(function() {
	orm_prefs = orm_state_get('prefs', {});
	orm_prefs_prev = orm_state_get('prefs_prev', {});

	$("section#prefs-lookandfeel > form").append(
		orm_prefs_combine2(orm_prefs_boolean("highlight_moves", "Highlight legal moves"),
						   orm_prefs_boolean("highlight_check", "Highlight check")),
		orm_prefs_combine2(orm_prefs_boolean("highlight_prev_move", "Highlight previous move"),
						   orm_prefs_number("board_max_size", "Maximum board size (px)")),
		orm_prefs_combine2(orm_prefs_number("board_animation_speed", "Move animation speed (ms)"),
						   orm_prefs_number("board_move_delay", "Delay before computer move (ms)")),
		orm_prefs_textarea("custom_css", "Custom CSS rules")
	);

	$("section#prefs-puzzlefilters > form").append(
		orm_prefs_combine2(orm_prefs_number("puzzle_depth_min", "Minimum puzzle depth"),
						   orm_prefs_number("puzzle_depth_max", "Maximum puzzle depth"))
	);

	$("section#prefs-uci > form").append(
		orm_prefs_number("uci_multipv", "Maximum number of lines"),
		orm_prefs_combine2(orm_prefs_string("uci_hard_limiter", "Analysis limiter"),
						   orm_prefs_string("uci_practice_limiter", "Practice limiter"))
	);

	$("div#preferences > section > form").each(function() {
		let input = $(document.createElement('input'));
		let span = $(document.createElement('span'));
		input.prop('type', 'submit');
		input.prop('value', 'Save preferences');
		input.addClass('btn btn-primary');
		span.text('Preferences saved!');
		span.addClass('ml-2 text-success');
		span.hide();
		input.click(function() {
			span.hide().fadeIn(250).delay(2000).fadeOut(1000);
			input.blur();
		});
		$(this).append($(document.createElement('div')).addClass('form-row').append(
			$(document.createElement('div')).addClass('col-md-3'),
			$(document.createElement('div')).addClass('col-md-9').append(input, span)
		));
	});

	$("div#preferences > section > form").submit(function(e) {
		e.preventDefault();

		$("div#preferences .save-pref").each(function() {
			let e = $(this);
			let k = e.data('k');
			if(typeof(k) === "undefined") return;

			if(e.hasClass('disabled')) {
				delete orm_prefs[k];
				if(typeof(e.data('prev-val')) !== "undefined") {
					orm_prefs_prev[k] = e.data('prev-val');
				} else {
					delete orm_prefs_prev[k];
				}
			} else {
				delete orm_prefs_prev[k];
				orm_prefs[k] = e.val();
			}
		});

		orm_state_set('prefs', orm_prefs);
		orm_state_set('prefs_prev', orm_prefs_prev);
		orm_prefs_apply();
	});

	orm_prefs_apply();
});

orm_when_puzzle_manifest_ready.push(function() {
	/* There's better ways, probably. */
	let tmap = {};
	let tags = [];

	for(let k in orm_prefs) {
		if(!k.match(/^puzzletag:/)) continue;
		let t = k.split(':', 2)[1];
		tmap[t] = true;
		tags.push(t);
	}

	for(let i in orm_manifest) {
		for(let t in orm_manifest[i].tags) {
			if(t in tmap) continue;
			tmap[t] = true;
			tags.push(t);
		}
	}

	tags.sort();
	let d = $("section#prefs-puzzlefilters > form > div.form-row:last-child");
	let d1 = null, d2 = null;

	for(let i in tags) {
		if(tags[i].match(/^(Depth|Min depth|Max depth) [1-9][0-9]*$/)) continue;

		let select = $(document.createElement('select'));
		let k = 'puzzletag:' + tags[i];
		select.append(
			$(document.createElement('option')).prop('value', '1').text('allowed'),
			$(document.createElement('option')).prop('value', '2').text('mandatory'),
			$(document.createElement('option')).prop('value', '0').text('forbidden')
		);
		ORM_PREFS_DEFAULTS[k] = '1';
		d2 = orm_prefs_generic(k, tags[i] + " tag", select);

		if(d1 === null) {
			d1 = d2;
		} else {
			d.before(orm_prefs_combine2(d1, d2));
			d1 = d2 = null;
		}
	}

	if(d1 !== null) d.before(d1);
});
