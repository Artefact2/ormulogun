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

const orm_error = function(str) {
	let err = $(document.createElement('p'));
	err.addClass('alert alert-danger');
	err.text(str);
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

	$("button#start").click(function() {
		$("div#intro").fadeOut(250, function() {
			$("div#select-puzzleset").fadeIn(250);
			history.replaceState(null, null, "#list");
		});
	}).removeAttr('disabled').removeClass('disabled');

	$("p#enable-js").remove();
};

const orm_try_init = function() {
	if(typeof(Module) !== "undefined") {
		orm_init();
	} else {
		setTimeout(orm_try_init, 10);
	}
};

$(orm_try_init);
