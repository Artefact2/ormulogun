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

let orm_engine = null;
let orm_practice = false;
let orm_analyse = false;

const orm_uci_handle_message = function(msg) {
	if(msg === "uciok") {
		$("p#loading-engine").remove();
		return;
	}

	if(orm_analyse === true && msg.match(/^info /)) {
		let idx = msg.match(/\smultipv\s+([1-9][0-9]*)\s/, msg);
		let score = msg.match(/\sscore\s+(cp|mate) (-?[1-9][0-9]*)\s/);
		let pv = msg.match(/\spv\s+(.+)$/);

		let li = $("div#engine-stuff > ul > li").eq(parseInt(idx[1], 10) - 1);
		li.text(score[1] + ' ' + score[2] + ' ' + pv[1]);
		return;
	}

	if(orm_practice !== false && msg.match(/^bestmove /)) {
		let lan = msg.split(' ', 3)[1]; /* XXX: handle checkmate */
		orm_do_legal_move(lan, true, null, undefined, undefined, orm_get_board());
	}
};

const orm_uci_init = function() {
	if(orm_engine === null) {
		let p = $(document.createElement('p'));
		p.attr('id', 'loading-engine').addClass('alert alert-primary').text('Loading stockfish.js…');
		$("nav#mainnav").after(p);

		if(typeof WebAssembly === "object" && WebAssembly.validate(Uint8Array.of(0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00))) {
			orm_engine = new Worker('stockfish.wasm.js');
		} else {
			orm_engine = new Worker('stockfish.js');
		}

		orm_engine.addEventListener("message", function(e) {
			orm_uci_handle_message(e.data);
		});
	}

	orm_engine.postMessage('uci');
};

const orm_uci_go = function() {
	if(orm_analyse === false) return;

	let ul = $("div#engine-stuff > ul");
	let n = parseInt(orm_pref('uci_multipv'), 10);
	ul.empty();
	for(let i = 0; i < n; ++i) {
		ul.append($(document.createElement('li')));
	}
	orm_engine.postMessage('stop');
	orm_engine.postMessage('setoption name MultiPV value ' + orm_pref('uci_multipv'));
	orm_engine.postMessage('position fen ' + orm_get_board().data('fen'));
	orm_engine.postMessage('go ' + orm_pref('uci_hard_limiter'));
};

const orm_uci_go_practice = function() {
	if(orm_practice === false) return;

	orm_engine.postMessage('stop');
	orm_engine.postMessage('setoption name MultiPV value 1');
	orm_engine.postMessage('position fen ' + orm_get_board().data('fen'));
	orm_engine.postMessage('go ' + orm_pref('uci_practice_limiter'));
};

orm_when_ready.push(function() {
	$("button#engine-analyse, button#engine-practice").each(function() {
		let t = $(this);
		t.data('start-text', t.text());
	});

	$("button#engine-analyse").click(function() {
		let t = $(this);

		if(orm_practice !== false) $("button#engine-practice").click();
		if(orm_analyse === true) {
			orm_analyse = false;
			orm_engine.postMessage('stop');
			t.text(t.data('start-text'));
			t.removeClass('btn-secondary').addClass('btn-outline-secondary');
			return;
		}

		orm_analyse = true;
		t.text('Stop analysis');
		t.addClass('btn-secondary').removeClass('btn-outline-secondary');
		orm_uci_init();
		orm_uci_go();
	});

	$("button#engine-practice").click(function() {
		let t = $(this);
		let b = orm_get_board();

		if(orm_analyse === true) $("button#engine-analyse").click();
		if(orm_practice !== false) {
			orm_practice = false;
			orm_engine.postMessage('stop');
			t.text(t.data('start-text'));
			t.removeClass('btn-secondary').addClass('btn-outline-secondary');
			return;
		}

		orm_practice = b.hasClass('white') ? 'black' : 'white';
		t.text('Stop practicing');
		t.addClass('btn-secondary').removeClass('btn-outline-secondary');
		$("div#engine-stuff > ul").empty();
		orm_uci_init(); /* XXX: race condition with go called from move.js? */
	});
});
