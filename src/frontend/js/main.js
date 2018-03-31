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

"use strict";

let orm_when_ready = [];
let orm_when_puzzle_manifest_ready = [];

const orm_error = function(str) {
	let err = $(document.createElement('p'));
	err.addClass('alert alert-danger alert-dismissable fade show');
	err.text(str);
	err.append(
		$(document.createElement('button'))
			.addClass('close')
			.attr('data-dismiss', 'alert')
			.prop('title', 'Close this alert')
			.prop('type', 'button')
			.append($(document.createElement('span')).text('×'))
	);
	$("nav#mainnav").after(err);
};

const orm_init = function() {
	for(let i in orm_when_ready) orm_when_ready[i]();

	$(window).resize(function() {
		let x = $(window).width() * .9;
		let y = $(window).height() - $("nav#mainnav").height() - 30;
		let width = Math.floor(Math.min(x, y));
		width -= width % 8; /* Prevent aliasing issues with background */
		width = width.toString();
		$("div.board.board-main").css("width", width);
	}).resize();

	$("p#enable-js").remove();

	$("a#progress-save").click(function(e) {
		let t = $(this);
		let b = new Blob([ JSON.stringify(localStorage) ], { type: 'application/json' });
		let uri = URL.createObjectURL(b);
		if(t.prop('href') !== '#') URL.revokeObjectURL(t.prop('href'));
		t.prop('href', uri);
		t.prop('download', 'ormulogun-' + Date.now().toString() + '.json');
	});

	$("a#progress-reset").click(function(e) {
		e.preventDefault();
		if(prompt("Do you really want to reset all your progress? Type uppercase 'reset' to confirm.") === "RESET") {
			for(let k in localStorage) orm_state_unset(k);
			orm_upgrade_state();
			orm_generate_journal_page();
		}
	});

	$("a#progress-merge").click(function(e) {
		e.preventDefault();
		$("input#progress-load-file").click();
	});

	$("input#progress-load-file").change(function() {
		let merge = function(rs) {
			rs = JSON.parse(rs);
			orm_upgrade_state(rs);

			for(let k in rs) {
				if(k.match(/^journal_/)) {
					orm_state_set(k, orm_journal_merge(orm_state_get(k, []), JSON.parse(rs[k])));
				} else if(k.match(/^leitner_/)) {
					orm_state_set(k, orm_merge_leitner_boxes(orm_state_get(k, {}), JSON.parse(rs[k])));
				} else if(typeof(orm_state_get(k, undefined)) === "undefined") {
					orm_state_set(k, JSON.parse(rs[k]));
				}
				orm_generate_journal_page(); /* XXX: inefficient */
			}
		};
		for(let i = 0; i < this.files.length; ++i) {
			let r = new FileReader();
			r.onload = function() {
				merge(r.result);
			};
			r.readAsText(this.files[i]);
		}
	});
};

const orm_try_init = function() {
	if(typeof(Module) !== "undefined") {
		orm_init();
	} else {
		setTimeout(orm_try_init, 10);
	}
};

$(orm_try_init);
