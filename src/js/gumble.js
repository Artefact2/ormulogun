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

const GUMBLE_BOARD_SIZE = 256; /* >= sizeof(cch_board_t) */
const GUMBLE_MOVE_SIZE = 4; /* == sizeof(cch_move_t) */
const GUMBLE_MOVELIST_LENGTH = 80; /* >= CCH_MOVELIST_LENGTH */
const GUMBLE_SAFE_FEN_LENGTH = 90; /* >= SAFE_FEN_LENGTH */
const GUMBLE_SAFE_ALG_LENGTH = 10; /* >= SAFE_ALG_LENGTH */

let gumble_board = null;
let gumble_fen_str = null;
let gumble_move_str = null;
let gumble_move = null;
let gumble_movelist = null;

const gumble_load_fen = function(fen) {
	writeAsciiToMemory(fen, gumble_fen_str);
	Module._cch_load_fen(gumble_board, gumble_fen_str);
};

const gumble_save_fen = function() {
	Module._cch_save_fen(gumble_board, gumble_fen_str, GUMBLE_SAFE_FEN_LENGTH);
	return Pointer_stringify(gumble_fen_str);
};

const gumble_is_move_legal = function(lan) {
	writeAsciiToMemory(lan, gumble_move_str);
	Module._cch_parse_lan_move(gumble_move_str, gumble_move);
	return Module._cch_is_move_legal(gumble_board, gumble_move);
};

const gumble_play_legal_lan = function(lan) {
	writeAsciiToMemory(lan, gumble_move_str);
	Module._cch_parse_lan_move(gumble_move_str, gumble_move);
	Module._cch_play_legal_move(gumble_board, gumble_move, 0);
};

const gumble_lan_to_san = function(lan) {
	writeAsciiToMemory(lan, gumble_move_str);
	Module._cch_parse_lan_move(gumble_move_str, gumble_move);
	Module._cch_format_san_move(gumble_board, gumble_move, gumble_move_str, GUMBLE_SAFE_ALG_LENGTH, 1);
	return Pointer_stringify(gumble_move_str);
};

const gumble_is_own_king_checked = function(sq) {
	/* XXX: relies a lot on structure packing, padding, and ordering */
	let side = Module.getValue(gumble_board + 196, 'i8');
	let k = Module.getValue(gumble_board + 192 + side, 'i8');
	return Module._cch_is_square_checked(gumble_board, k);
};

const gumble_smoves = function() {
	return Module.getValue(gumble_board + 188, 'i32');
};

orm_when_ready.push(function() {
	gumble_board = Module._malloc(GUMBLE_BOARD_SIZE);
	gumble_fen_str = Module._malloc(GUMBLE_SAFE_FEN_LENGTH);
	gumble_move_str = Module._malloc(GUMBLE_SAFE_ALG_LENGTH);
	gumble_move = Module._malloc(GUMBLE_MOVE_SIZE);
	gumble_movelist = Module._malloc(GUMBLE_MOVE_SIZE * GUMBLE_MOVELIST_LENGTH);
	Module._cch_init_board(gumble_board);
});
