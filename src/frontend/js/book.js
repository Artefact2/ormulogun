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

let orm_eco_book = {};
let orm_book = null;
let orm_book_open = false;

const orm_book_update = function() {
	if(orm_book === null) return;

	let ul = $("div#book-stuff > ul").empty();
	let fen = orm_get_board().data('fen').split(' ');
	let bfen = fen.slice(0, 4).join(' ');

	if(!(bfen in orm_book)) return;

	let n = orm_book[bfen][0] + orm_book[bfen][1] + orm_book[bfen][2];
	let prefix = fen[5] + (fen[1] === 'b' ? '... ' : '. ');

	let scale = function(p) {
		return -100.0 * Math.log(p / 0.01) / Math.log(0.01);
	};

	for(var i in orm_book[bfen][3]) {
		let li = $(document.createElement('li'));
		let entry = orm_book[bfen][3][i];

		let san = gumble_lan_to_san(entry[3]);
		li.addClass('row pv');
		li.data('pv', entry[3]);
		li.append($(document.createElement('div')).addClass('col-3 p-0').append($(document.createElement('strong')).text(prefix + san)));

		let t = entry[0] + entry[1] + entry[2];
		let p = t / n;
		let ci = 256.0 * Math.sqrt(p * (1.0 - p) / n); p *= 100;
		li.append($(document.createElement('div')).addClass('col-4 pl-0 pr-1').append($(document.createElement('div')).addClass('progress').append(
			$(document.createElement('div')).addClass('progress-bar').css('width', (p - ci).toString() + '%'),
			$(document.createElement('div')).addClass('progress-bar ci').css('width', (2 * ci).toString() + '%')
		).prop('title', p.toFixed(2) + '±' + ci.toFixed(2) + '% [' + orm_format_integer(t) + ']')));

		let wr = entry[0] / t;
		let dr = entry[1] / t;
		let br = entry[2] / t;
		let wci = 256.0 * Math.sqrt(wr * (1.0 - wr) / t); wr *= 100;
		let dci = 256.0 * Math.sqrt(dr * (1.0 - dr) / t); dr *= 100;
		let bci = 256.0 * Math.sqrt(br * (1.0 - br) / t); br *= 100;
		li.append($(document.createElement('div')).addClass('col-5 p-0').append($(document.createElement('div')).addClass('progress').append(
			$(document.createElement('div')).addClass('progress-bar white').css('width', (wr - wci).toString() + '%'),
			$(document.createElement('div')).addClass('progress-bar ci-wd').css('width', (wci + dci / 2.0).toString() + '%'),
			$(document.createElement('div')).addClass('progress-bar draw') .css('width', (dr - dci).toString() + '%'),
			$(document.createElement('div')).addClass('progress-bar ci-db').css('width', (bci + dci / 2.0).toString() + '%'),
			$(document.createElement('div')).addClass('progress-bar black').css('width', (br - bci).toString() + '%')
		).prop(
			'title',
			'White wins\t' + wr.toFixed(2) + '±' + wci.toFixed(2) + '% [' + orm_format_integer(entry[0]) + ']\n'
				+ 'Draws\t\t\t' + dr.toFixed(2) + '±' + dci.toFixed(2) + '% [' + orm_format_integer(entry[1]) + ']\n'
				+ 'Black wins\t\t' + br.toFixed(2) + '±' + bci.toFixed(2) + '% [' + orm_format_integer(entry[2]) + ']'
		)));

		ul.append(li);
	}
};

const orm_book_close = function() {
	if(orm_book_open === false) return;
	$("button#book-toggle").click();
};

orm_when_ready.push(function() {
	$.ajax({
		url: './eco.tsv.js',
		dataType: 'text'
	}).done(function(tsv) {
		let rows = tsv.split('\n');
		let nrows = rows.length - 1; /* Do not include trailing newline */
		for(let i = 0; i < nrows; ++i) {
			let fields = rows[i].split('\t');
			let name = fields[1];
			if(fields[0] !== '') name = fields[0] + ' ' + name;
			if(fields[2] !== '') name = name + ' ' + fields[2];
			orm_eco_book[fields[3]] = name;
		}
	});

	$("button#book-toggle").click(function() {
		let btn = $(this);
		let d = $("div#book-stuff");

		if(orm_book_open === true) {
			orm_book_open = false;
			d.hide();
			btn.text(btn.data('start-text'));
			return;
		}

		btn.data('start-text', btn.text());
		btn.text('Close book');

		let work = function() {
			orm_book_open = true;
			orm_book_update();
			d.show();
		};

		if(orm_book === null) {
			let p = $(document.createElement('p')).addClass('alert alert-primary container-fluid');
			p.text('Loading the opening book…');
			d.prepend(p);

			$.ajax({ url: './book.tsv.js', dataType: 'text' }).always(function() {
			}).fail(function() {
				p.toggleClass('alert-primary alert-danger').text('Could not fail the opening book.');
				orm_book = {};
			}).done(function(tsv) {
				orm_book = {};
				let rows = tsv.split('\n');
				let nrows = rows.length - 1;
				for(let i = 0; i < nrows; ++i) {
					let fields = rows[i].split('\t');
					if(fields[4] === '') {
						orm_book[fields[3]] = [ parseInt(fields[0], 10), parseInt(fields[1], 10), parseInt(fields[2], 10), [] ];
					} else {
						orm_book[fields[3]][3].push([ parseInt(fields[0], 10), parseInt(fields[1], 10), parseInt(fields[2], 10), fields[4] ]);
					}
				}
				for(let p in orm_book) {
					orm_book[p][3].sort(function(a, b) {
						return b[0] + b[1] + b[2] - a[0] - a[1] - a[2];
					});
				}

				p.remove();
				work();
			});
		} else {
			work();
		}
	});
});
