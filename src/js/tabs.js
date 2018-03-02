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

const orm_load_tab = function(id, anchor, animate, after) {
	if(typeof(animate) === "undefined") animate = true;

	let work = function() {
		if(anchor !== null) {
			history.replaceState(null, null, anchor);
		}
		if(after) after();
	};

	let visible = $("body > div.tab").filter(':visible');
	let want = $("body > div.tab#" + id);

	$("nav#mainnav").removeClass('bg-success bg-danger');

	if(!animate) {
		visible.hide();
		want.show();
		work();
		return;
	}

	visible.fadeOut(250, function() {
		want.fadeIn(250, work);
	});
};

orm_when_ready.push(function() {
	$("[data-orm-tab]").click(function(e) {
		let t = $(this);
		orm_load_tab(t.data('orm-tab'), t.data('orm-anchor') || t.prop('href'));
		e.preventDefault();
		t.blur();
	});
});
