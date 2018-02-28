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
	"custom_css": "",
};

let orm_prefs = null;
let orm_prefs_prev = null;


const orm_prefs_generic = function(section, k, lstr, element) {
	let d = $(document.createElement('div'));
	let label = $(document.createElement('label'));
	let override = $(document.createElement('input'));
	let olabel = $(document.createElement('label'));
	let group = $(document.createElement('div'));

	element.addClass('form-control save-pref');
	element.data('k', k);

	d.addClass('form-row form-group');
	group.addClass('input-group');

	label.text(lstr);
	label.prop('for', element.prop('id'));

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

	d.append(label.addClass('col-md-3 col-form-label'));
	d.append($(document.createElement('div')).addClass('col-md-9').append(group));
	$("section#" + section).append(d);
};

const orm_prefs_boolean = function(section, k, lstr) {
	let select = $(document.createElement('select'));
	select.addClass('custom-select');
	select.prop('id', 's_' + k);
	select.append($(document.createElement('option')).prop('value', 1).text('yes'));
	select.append($(document.createElement('option')).prop('value', 0).text('no'));
	return orm_prefs_generic(section, k, lstr, select);
};

const orm_prefs_number = function(section, k, lstr) {
	let input = $(document.createElement('input'));
	input.prop('id', 's_' + k);
	input.prop('type', 'number');
	return orm_prefs_generic(section, k, lstr, input);
};

const orm_prefs_textarea = function(section, k, lstr) {
	let ta = $(document.createElement('textarea'));
	ta.prop('id', 's_' + k);
	return orm_prefs_generic(section, k, lstr, ta);
}

orm_when_ready.push(function() {
	orm_prefs = orm_state_get('prefs', {});
	orm_prefs_prev = orm_state_get('prefs_prev', {});

	orm_prefs_boolean("lookandfeel", "highlight_moves", "Highlight legal moves");
	orm_prefs_boolean("lookandfeel", "highlight_check", "Highlight check");
	orm_prefs_boolean("lookandfeel", "highlight_prev_move", "Highlight previous move");
	orm_prefs_number("lookandfeel", "board_max_size", "Maximum board size (pixels, 0=unlimited)");
	orm_prefs_textarea("lookandfeel", "custom_css", "Custom CSS rules");

	$("div#preferences > form > section").each(function() {
		let input = $(document.createElement('input'));
		input.prop('type', 'submit');
		input.prop('value', 'Save preferences');
		input.addClass('btn btn-primary');
		input.click(function() {
			let span = $(document.createElement('span'));
			span.text('Preferences saved!');
			span.addClass('ml-2 text-success');
			input.find('+ span').remove();
			input.after(span);
			span.hide().fadeIn(250).delay(2000).fadeOut(1000);
			input.blur();
		});
		$(this).append($(document.createElement('div')).addClass('form-row').append(
			$(document.createElement('div')).addClass('col-md-9').append(input)
		));
	});

	$("div#preferences > form").submit(function(e) {
		e.preventDefault();

		$(this).find('.save-pref').each(function() {
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
		orm_prefs_css_update();
	});

	orm_prefs_css_update();
});

const orm_prefs_css_update = function() {
	let css = '';

	if("custom_css" in orm_prefs) css += orm_prefs.custom_css;
	if("highlight_moves" in orm_prefs && orm_prefs.highlight_moves === "0") {
		css += "div.board > div.back.move-source:before, div.board > div.back.move-target:before { content: unset; }";
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

	let style = $("head > style#prefs");
	if(style.length === 0) {
		style = $(document.createElement('style'));
		style.prop('id', 'prefs');
		$('head').append(style);
	}
	style.text(css);
};
