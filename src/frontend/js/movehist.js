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
	$("button#movehist-first, button#movehist-prev, button#movehist-next, button#movehist-last")
		.prop('disabled', 'disabled').addClass('disabled');
};

const orm_movehist_in_rootline = function(e) {
	return e.parent().prop('id') === "movehist";
};

const orm_movehist_current = function() {
	return $("ul#movehist li.current");
};

const orm_movehist_next = function(e) {
	if(e.length === 0) return $("ul#movehist > li > button").not(":disabled").first().parent();

	e = e.next();
	while(e.hasClass('dummy') || e.hasClass('variation')) {
		e = e.next();
	}
	return e;
};

const orm_movehist_prev = function(e) {
	let prev = e.prev();

	while(prev.hasClass('dummy') || prev.hasClass('variation')) {
		prev = prev.prev();
	}

	if(prev.length !== 0) return prev;

	if(e.parent().parent().hasClass('variation')) {
		return orm_movehist_prev(e.parent().parent());
	} else {
		return $();
	}
};

const orm_movehist_last = function(e) {
	if(e.length === 0) return $("ul#movehist > li > button").not(":disabled").last().parent();
	return e.parent().children('li').last();
}

const orm_movehist_make_active = function(e) {
	const prev = orm_movehist_current().removeClass('current');
	const prevbtn = prev.children('button');
	prevbtn.removeClass('btn-primary btn-success btn-danger').addClass('btn-light');
	if(prev.hasClass('good-move')) {
		prevbtn.addClass('text-success');
	} else if(prev.hasClass('bad-move')) {
		prevbtn.addClass('text-danger');
	}

	if(e.length === 0) {
		$("button#movehist-first, button#movehist-prev").prop('disabled', 'disabled').addClass('disabled');
		$("button#movehist-next, button#movehist-last").prop('disabled', false).removeClass('disabled');
		orm_get_board().removeClass('game-over'); /* XXX */
		return;
	}

	e.addClass('current');

	$("button#movehist-first, button#movehist-prev").prop('disabled', false).removeClass('disabled');
	if(orm_movehist_next(e).length === 0) {
		$("button#movehist-next, button#movehist-last").prop('disabled', 'disabled').addClass('disabled');
	} else {
		$("button#movehist-next, button#movehist-last").prop('disabled', false).removeClass('disabled');
	}

	const btn = e.children('button');
	btn.removeClass('btn-light');
	if(e.hasClass('good-move')) {
		btn.removeClass('text-success').addClass('btn-success');
	} else if(e.hasClass('bad-move')) {
		btn.removeClass('text-danger').addClass('btn-danger');
	} else {
		btn.addClass('btn-primary');
	}

	orm_get_board().toggleClass('game-over', e.hasClass('game-over')); /* XXX */

	/* Try to scroll to the center, see https://stackoverflow.com/a/33193694/615776 */
	let p = $("ul#movehist");
	p.scrollTop(p.scrollTop() + e.position().top - .5 * p.height() + .5 * e.height());
};

const orm_movehist_push = function(startfen, lan, san) {
	const mh = $("ul#movehist");
	if(mh.children().length === 0 && startfen.split(' ', 3)[1] === 'b') {
		/* Insert dummy button for turn counter */
		let li = $(document.createElement('li'));
		let tn = $(document.createElement('span'));
		let btn = $(document.createElement('button'));
		li.append(tn, btn);
		li.addClass('dummy col-6');
		tn.text(startfen.split(' ', 6)[5] + ". ");
		btn.text('…');
		btn.addClass('btn btn-sm btn-light disabled');
		btn.prop('disabled', 'disabled');
		mh.append(li);
	}

	mh.find('li.new').removeClass('new');

	let current = orm_movehist_current();
	let mainnext = current.length === 0 ? mh.children('li').first() : current.next();
	let variations = $();
	while(mainnext.hasClass('dummy') || mainnext.hasClass('variation')) {
		if(mainnext.hasClass('variation')) {
			variations = variations.add(mainnext.children('ul').first().children('li').first());
		}
		mainnext = mainnext.next();
	}

	if(mainnext.data('fen') === startfen && mainnext.data('lan') === lan) {
		orm_movehist_make_active(mainnext);
		return;
	}

	if(variations.filter(function() {
		let li = $(this);
		if(li.data('fen') === startfen && li.data('lan') === lan) {
			orm_movehist_make_active(li);
			return true;
		}
		return false;
	}).length !== 0) return;

	let li = $(document.createElement('li'));
	let btn = $(document.createElement('button'));
	li.append(btn);
	li.data('fen', startfen).data('lan', lan);
	li.addClass('new mb-1');
	btn.addClass('btn btn-sm');
	btn.text(san);
	btn.prop('type', 'button');

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
			li.addClass('mr-1');
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
		vli.addClass('variation w-100');
		li.addClass('mr-1');

		if(li.hasClass('black')) vli.addClass('black');

		if(current.next().hasClass('dummy')) {
			current.next().after(vli);
		} else {
			if(current.length === 0) {
				let first = mh.find('> li > button').not(':disabled').first().parent();
				if(first.length !== 0) {
					first.before(vli);
				} else {
					mh.prepend(vli);
				}
			} else {
				current.after(vli);
			}

			if(rootline && li.hasClass('black') && current.length !== 0 && !(current.next().hasClass('dummy'))) {
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

	if(Module._cch_generate_moves(gumble_board, gumble_movelist, 0, 0, 64) === 0) {
		li.addClass('game-over');
		let sp = $(document.createElement('small')).addClass('eg ml-1');
		li.append(sp);

		if(gumble_is_own_king_checked()) {
			if(li.hasClass('black')) {
				sp.prop('title', 'Checkmate, Black wins.').text('0 – 1');
			} else {
				sp.prop('title', 'Checkmate, White wins.').text('1 – 0');
			}
		} else {
			sp.prop('title', 'Draw caused by stalemate.').text('½ – ½');
		}
	} else if(gumble_smoves() > 100) {
		/* XXX: refactor this boilerplate */
		li.addClass('game-over');
		let sp = $(document.createElement('small')).addClass('eg ml-1');
		li.append(sp);
		sp.prop('title', 'Draw caused by 50-move rule.').text('½ – ½');
	} else {
		let endpos = gumble_save_fen().split(' ', 2)[0]; /* XXX: is this exactly correct? */
		li.data('endpos', endpos);
		li.data('smoves', gumble_smoves());

		let l = li, pp = 1;
		while((l = orm_movehist_prev(l)) && l.length > 0) {
			if(l.data('endpos') === endpos) ++pp;
			if(pp === 3 || l.data('smoves') === 0) break;
		}

		if(pp === 3) {
			/* XXX: refactor this boilerplate */
			li.addClass('game-over');
			let sp = $(document.createElement('small')).addClass('eg ml-1');
			li.append(sp);
			sp.prop('title', 'Draw caused by threefold repetition.').text('½ – ½');
		}
	}

	orm_movehist_make_active(li);
};

const orm_movehist_merge_from_puzzle_step = function(step) {
	if(!Array.isArray(step)) return;

	let current = orm_movehist_current();
	let fen = gumble_save_fen(); /* XXX: undo move */

	let san = gumble_lan_to_san(step[0]);
	gumble_play_legal_lan(step[0]);
	orm_movehist_push(fen, step[0], san);

	let current2 = orm_movehist_current();
	let fen2 = gumble_save_fen();

	for(let lan in step[1]) {
		san = gumble_lan_to_san(lan);
		gumble_play_legal_lan(lan);
		orm_movehist_push(fen2, lan, san);
		orm_movehist_current().addClass('good-move');

		orm_movehist_merge_from_puzzle_step(step[1][lan]);

		gumble_load_fen(fen2);
		orm_movehist_make_active(current2);
	}

	gumble_load_fen(fen);
	orm_movehist_make_active(current);
};

const orm_movehist_merge_from_puzzle = function(puz) {
	let current = orm_movehist_current();
	let fen = gumble_save_fen();

	gumble_load_fen(puz[0]);
	orm_movehist_make_active($());
	orm_movehist_merge_from_puzzle_step(puz[1]);

	gumble_load_fen(fen);
	orm_movehist_make_active(current);
};

orm_when_ready.push(function() {
	$("body").keydown(function(e) {
		if(!$("div#board").is(':visible')) return;
		let tgt = null;

		switch(e.which) {
		case 37: /* Left */
			tgt = $("button#movehist-prev");
			break;

		case 38: /* Up */
			tgt = $("button#movehist-first");
			break;

		case 39: /* Right */
			tgt = $("button#movehist-next");
			break;

		case 40: /* Down */
			tgt = $("button#movehist-last");
			break;

		default: return;
		}
		if(tgt === null || tgt.hasClass("disabled") || tgt.prop("disabled")) return;
		tgt.click();
		e.preventDefault();
	});

	orm_get_board().on("wheel", function(e) {
		let tgt = null;
		if(e.originalEvent.deltaY > 0) {
			/* Scroll down */
			tgt = $("button#movehist-next");
		} else if(e.originalEvent.deltaY < 0) {
			tgt = $("button#movehist-prev");
		}
		if(tgt === null || tgt.hasClass("disabled") || tgt.prop("disabled")) return;
		tgt.click();
		e.preventDefault();
	});

	$("button#movehist-next").click(function() {
		orm_movehist_next(orm_movehist_current()).children('button').click();
	});

	$("button#movehist-prev").click(function() {
		let cur = orm_movehist_current();
		cur.data('reverse', true);
		cur.children('button').click();
	});

	$("button#movehist-last").click(function() {
		orm_movehist_last(orm_movehist_current()).children('button').click();
	});

	$("button#movehist-first").click(function() {
		let first = $("ul#movehist > li > button").not(":disabled").first().parent();
		first.data('reverse', true);
		first.find('button').click();
	});

	$("ul#movehist").on('click', 'li > button', function() {
		let btn = $(this);
		let li = btn.parent();
		let reverse = li.data('reverse');
		li.data('reverse', false);
		orm_movehist_make_active(reverse ? orm_movehist_prev(li) : li);
		gumble_load_fen(li.data('fen'));
		orm_do_legal_move(li.data('lan'), true, null, false, reverse, orm_get_board());
	});
});
