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

const orm_movehist_reset = function() {
	$("ul#movehist").empty();
};

const orm_movehist_push = function(startfen, lan, san) {
	const mh = $("ul#movehist");
	if(mh.children().length === 0 && startfen.split(' ', 3)[1] === 'b') {
		/* Insert dummy button for turn counter */
		let li = $(document.createElement('li'));
		let tn = $(document.createElement('span'));
		let btn = $(document.createElement('btn'));
		li.append(tn, btn);
		li.addClass('dummy col-6');
		tn.text(startfen.split(' ', 6)[5] + ". ");
		btn.text('…');
		btn.addClass('btn btn-light disabled');
		btn.prop('disabled', 'disabled');
		mh.append(li);
	}

	let current = mh.find("li > button.btn-primary").parent();
	current.children('button').removeClass('btn-primary').addClass('btn-light');

	let mainnext = current.next();
	let variations = $();
	while(mainnext.hasClass('dummy') || mainnext.hasClass('variation')) {
		if(mainnext.hasClass('variation')) {
			variations = variations.add(mainnext.children('ul').first().children('li').first());
		}
		mainnext = mainnext.next();
	}

	if(mainnext.data('fen') === startfen && mainnext.data('lan') === lan) {
		mainnext.children('button').addClass('btn-primary').removeClass('btn-light');
		return;
	}

	if(variations.filter(function() {
		let li = $(this);
		if(li.data('fen') === startfen && li.data('lan') === lan) {
			li.children('button').addClass('btn-primary').removeClass('btn-light');
			return true;
		}
		return false;
	}).length !== 0) return;

	let li = $(document.createElement('li'));
	let btn = $(document.createElement('button'));
	li.append(btn);
	li.data('fen', startfen).data('lan', lan);
	btn.addClass('btn btn-primary');
	btn.text(san);

	if(startfen.split(' ', 3)[1] === 'w') {
		li.addClass('white');
		let tn = $(document.createElement('span'));
		li.prepend(tn);
		tn.text(startfen.split(' ', 6)[5] + ". ");
	} else {
		li.addClass('black');
	}

	let rootline = current.length === 0 || current.parent().prop('id') === "movehist";

	if(mainnext.length === 0) {
		/* Add main line move */

		if(rootline) {
			li.addClass('col-6');
		} else {
			btn.addClass('btn-sm');
		}

		if(current.length === 0) {
			mh.append(li);
		} else {
			current.after(li);
		}
	} else {
		/* Add variation */
		let vli = $(document.createElement('li'));
		let ul = $(document.createElement('ul'));
		vli.append(ul);
		ul.append(li);
		vli.addClass('variation w-100 ml-4');
		li.data('fen', startfen).data('lan', lan);
		btn.addClass('btn-sm');

		if(li.hasClass('black')) vli.addClass('black');

		if(current.next().hasClass('dummy')) {
			current.next().after(vli);
		} else {
			current.after(vli);

			if(rootline && li.hasClass('black') && !(current.next().hasClass('dummy'))) {
				let dummy = $(document.createElement('li'));
				let btn = $(document.createElement('button'));
				dummy.append(btn);
				dummy.addClass('dummy col-6');
				btn.addClass('btn btn-light disabled').text('…').prop('disabled', 'disabled');

				vli.before(dummy.clone());
				vli.after(dummy);
			}
		}
	}
};

orm_when_ready.push(function() {
	$("ul#movehist").on('click', 'li > button', function() {
		let btn = $(this);
		let li = btn.parent();
		$("ul#movehist li > button.btn-primary").removeClass('btn-primary').addClass('btn-light');
		btn.removeClass('btn-light').addClass('btn-primary');
		orm_load_fen(li.data('fen'));
		gumble_load_fen(li.data('fen'));
		setTimeout(function() {
			orm_do_legal_move(li.data('lan'), true, null, false);
		}, 50);
	});
});
