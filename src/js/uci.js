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
let orm_engine_going = false;
let orm_engine_loaded_once = false;
let orm_practice = false;
let orm_analyse = false;
let orm_analyse_fen = null;
let orm_score_sign = 1;

let orm_uci_stop_timeout = null;
let orm_uci_stop_payload = null;

const orm_uci_format_node_count = function(nc) {
	if(nc > 10000000) {
		return (nc / 1000000.0).toFixed(1).toString() + 'M';
	} else if(nc > 10000) {
		return (nc / 1000.0).toFixed(1).toString() + 'K';
	}

	return nc.toString();
};

const orm_format_san_pv = function(pv) {
	gumble_load_fen(orm_analyse_fen);
	let ret = [];
	pv = pv.split(' ');

	for(let i in pv) {
		ret.push(gumble_lan_to_san(pv[i]));
		gumble_play_legal_lan(pv[i]);
	}

	return ret.join(' ');
};

const orm_uci_handle_message = function(msg) {
	if("data" in msg) msg = msg.data;

	if(msg === "uciok") {
		$("p#loading-engine").remove();
		return;
	}

	if(orm_analyse === true && msg.match(/^info\s/)) {
		let depth = msg.match(/\sdepth\s+([1-9][0-9]*)\s/);
		let nodes = msg.match(/\snodes\s+([1-9][0-9]*)\s/);
		let nps = msg.match(/\snps\s+([1-9][0-9]*)\s/);

		if(depth) $("div#engine-depth").text("depth " + depth[1]);
		if(nodes) $("div#engine-nodes").text(orm_uci_format_node_count(parseInt(nodes[1], 10)) + " nodes");
		if(nps) $("div#engine-nps").text(orm_uci_format_node_count(parseInt(nps[1], 10)) + " nps");

		let m, p = 0.0;
		if(depth && (m = orm_pref('uci_hard_limiter').match(/^depth\s+([1-9][0-9]*)$/))) {
			p = parseInt(depth[1], 10) / parseInt(m[1], 10);
		} else if(nodes && (m = orm_pref('uci_hard_limiter').match(/^nodes\s+([1-9][0-9]*)$/))) {
			p = parseInt(nodes[1], 10) / parseInt(m[1], 10);
		} else if(m = orm_pref('uci_hard_limiter').match(/^movetime\s+([1-9][0-9]*)$/)) {
			let time = msg.match(/\stime\s+([1-9][0-9]*)\s/);
			if(time) {
				p = parseInt(time[1], 10) / parseInt(m[1], 10);
			}
		} else if(depth && nodes) {
			/* Fallback progress */
			p = parseInt(depth[1], 10) / 100.0 + parseInt(nodes[1], 10) / 1000000000.0;
		}
		$("div#analysis-stuff > div.progress > div.progress-bar").css('width', 95.0 * Math.min(1.0, p) + '%');

		let idx = msg.match(/\smultipv\s+([1-9][0-9]*)\s/, msg);
		let score = msg.match(/\sscore\s+(cp|mate) (0|-?[1-9][0-9]*)\s/);
		let pv = msg.match(/\spv\s+(.+)$/);
		if(!idx || !score || !pv) return;

		let li = $("div#analysis-stuff > ul > li").eq(parseInt(idx[1], 10) - 1).addClass('pv').data('pv', pv[1]);
		score[2] = parseInt(score[2], 10) * orm_score_sign;
		li.children('strong').text(
			score[1] === "cp" ? (
				score[2] === 0 ? '0' : ((score[2] > 0 ? '+' : '') + (score[2] / 100.0).toFixed(1).toString())
			) : '#' + score[2]);
		li.children('span').text(orm_format_san_pv(pv[1]));
		if(li.hasClass('active')) li.trigger('mouseleave').trigger('mouseenter');
		return;
	}

	if(msg.match(/^bestmove\s/)) {
		orm_uci_done_running();

		if(orm_practice !== false) {
			let lan = msg.split(' ', 3)[1]; /* XXX: handle checkmate */
			orm_do_legal_move(lan, true, null, undefined, undefined, orm_get_board());
		}

		return;
	}
};

const orm_uci_init = function() {
	if(orm_engine === null) {
		if(orm_engine_loaded_once === false) {
			let p = $(document.createElement('p'));
			p.attr('id', 'loading-engine').addClass('alert alert-primary').text('Loading stockfish.js…');
			$("nav#mainnav").after(p);
			orm_engine_loaded_once = true;
		}

		if(typeof WebAssembly === "object" && WebAssembly.validate(Uint8Array.of(0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00))) {
			orm_engine = new Worker('stockfish.wasm.js');
		} else {
			orm_engine = new Worker('stockfish.js');
		}

		orm_engine.addEventListener("message", orm_uci_handle_message);
	}

	orm_engine.postMessage('uci');
};

const orm_uci_prepare_running = function() {
	orm_engine_going = true;

	$("div#analysis-stuff > div.progress > div.progress-bar").css('width', '0%').removeClass('bg-success');
	$("div#analysis-stuff > ul > li.pv.active").trigger('mouseleave').addClass('active');
	$("div#analysis-stuff > ul > li").removeClass('pv').children().text('');
	$("button#engine-toggle").text('Stop').prop('disabled', false).removeClass('disabled');
};

const orm_uci_done_running = function() {
	orm_engine_going = false;

	if(orm_uci_stop_timeout !== null) {
		clearTimeout(orm_uci_stop_timeout);
		orm_uci_stop_timeout = null;
	}

	$("div#analysis-stuff > div.progress > div.progress-bar").css('width', '100%').addClass('bg-success');
	$("button#engine-toggle").text('Go deeper').prop('disabled', false).removeClass('disabled');

	if(orm_uci_stop_payload) {
		orm_uci_stop_payload();
		orm_uci_stop_payload = null;
	}
}

const orm_uci_stop = function(then) {
	if(!orm_engine_going) {
		if(then) then();
		return;
	}

	orm_engine.postMessage('stop');
	orm_uci_stop_payload = then;
	orm_uci_stop_timeout = setTimeout(function() {
		/* Engine didn't stop in time, kill it the hard way and spawn a new one */
		orm_engine.removeEventListener('message', orm_uci_handle_message);
		orm_engine.postMessage('quit');
		orm_engine = null;
		orm_uci_init();
		orm_uci_done_running();
	}, 500);
};

const orm_uci_stopall = function() {
	if(orm_analyse) $("button#engine-analyse").click();
	if(orm_practice) $("button#engine-practice").click();
	if(orm_engine) orm_uci_stop();
};

const orm_uci_go = function(limiter) {
	if(orm_analyse === false) return;
	if(typeof(limiter) === "undefined") limiter = orm_pref('uci_hard_limiter');

	orm_uci_stop(function() {
		orm_uci_prepare_running();
		orm_score_sign = orm_get_board().hasClass('white') ? 1 : - 1; /* XXX */
		orm_engine.postMessage('setoption name MultiPV value ' + orm_pref('uci_multipv'));
		orm_engine.postMessage('position fen ' + (orm_analyse_fen = orm_get_board().data('fen')));
		orm_engine.postMessage('go ' + limiter);
	});
};

const orm_uci_go_practice = function() {
	if(orm_practice === false) return;

	orm_uci_stop(function() {
		orm_uci_prepare_running();
		orm_engine.postMessage('setoption name MultiPV value 1');
		orm_engine.postMessage('position fen ' + orm_get_board().data('fen'));
		orm_engine.postMessage('go ' + orm_pref('uci_practice_limiter'));
	});
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
			orm_uci_stop();
			t.text(t.data('start-text'));
			t.removeClass('btn-secondary').addClass('btn-outline-secondary');
			$("div#analysis-stuff").hide();
			return;
		}

		orm_analyse = true;
		t.text('Stop analysis');
		t.addClass('btn-secondary').removeClass('btn-outline-secondary');
		$("div#analysis-stuff").show();
		orm_uci_init();
		orm_uci_go();
	});

	$("button#engine-practice").click(function() {
		let t = $(this);
		let b = orm_get_board();

		if(orm_analyse === true) $("button#engine-analyse").click();
		if(orm_practice !== false) {
			orm_practice = false;
			orm_uci_stop();
			t.text(t.data('start-text'));
			t.removeClass('btn-secondary').addClass('btn-outline-secondary');
			return;
		}

		orm_practice = b.hasClass('white') ? 'black' : 'white';
		t.text('Stop practicing');
		t.addClass('btn-secondary').removeClass('btn-outline-secondary');
		orm_uci_init(); /* XXX: race condition with go called from move.js? */
	});

	$("button#engine-toggle").click(function() {
		let t = $(this);
		t.prop('disabled', true).addClass('disabled').blur();
		if(orm_engine_going) {
			orm_uci_stop();
		} else {
			orm_uci_go('infinite');
		}
	});
});
