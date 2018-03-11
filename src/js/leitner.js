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

const orm_get_leitner_boxes = function(setid) {
	return orm_state_get("leitner_" + setid, {});
};

const orm_put_leitner_boxes = function(setid, b) {
	return orm_state_set("leitner_" + setid, b);
}

const orm_compute_puzzle_cooldown = function(box) {
	if(box === 0) return parseInt(orm_pref("leitner_cooldown_initial"), 10);
	return parseFloat(orm_pref("leitner_cooldown_increase_geometric")) * orm_compute_puzzle_cooldown(box - 1)
		+ parseInt(orm_pref("leitner_cooldown_increase_arithmetic"), 10);
};

const orm_get_puzzle_cooldown = function(setid, puzid) {
	let b = orm_get_leitner_boxes(setid);
	if(puzid in b) return b[puzid][1];
	return 0;
};

const orm_commit_puzzle_win = function(setid, puzid) {
	let t = Date.now();
	if(t < orm_get_puzzle_cooldown(setid, puzid)) return false;
	let b = orm_get_leitner_boxes(setid);

	if(!(puzid in b)) {
		b[puzid] = [ parseInt(orm_pref("leitner_first_win_initial"), 10), 0 ];
	}

	/* Puzzle won, goes in the next box */
	++b[puzid][0];
	b[puzid][1] = t + orm_compute_puzzle_cooldown(b[puzid][0]);
	orm_put_leitner_boxes(setid, b);
	orm_journal_push(setid, [ 1, puzid ]);
	return true;
};

const orm_commit_puzzle_loss = function(setid, puzid) {
	let t = Date.now();
	if(t < orm_get_puzzle_cooldown(setid, puzid)) return false;
	let b = orm_get_leitner_boxes(setid);

	/* Puzzle lost, goes back to the first box */
	b[puzid] = [ 0, t + orm_compute_puzzle_cooldown(0) ];
	orm_put_leitner_boxes(setid, b);
	orm_journal_push(setid, [ 0, puzid ]);
	return true;
};

const orm_find_candidate_puzzle = function(setid, puzlist) {
	let t = Date.now();
	let b = orm_get_leitner_boxes(setid);
	let candidates = [];
	let minbox = 10000000; /* XXX */
	let mt = parseInt(orm_pref("leitner_mastery_threshold"), 10);

	for(let puzid in b) {
		if(t < b[puzid][1]) continue;

		if(b[puzid][0] < minbox) {
			candidates = [];
			minbox = b[puzid][0];
		}

		if(orm_puzzle_filtered(puzlist[puzid])) {
			continue;
		}

		candidates.push(puzid);
	}

	/* Do we have non-mastered puzzles to repeat? */
	if(candidates.length > 0 && minbox < mt) {
		return parseInt(candidates[Math.floor(Math.random() * candidates.length)], 10);
	}

	/* Find a random puzzle that has never been attempted before. */
	/* XXX: this is probably slow and inefficient */
	let pl = puzlist.length, i = Math.floor(Math.random() * pl);
	for(let j = i; j < pl; ++j) {
		if(j in b || orm_puzzle_filtered(puzlist[j])) continue;
		return j;
	}
	for(let j = i - 1; j >= 0; --j) {
		if(j in b || orm_puzzle_filtered(puzlist[j])) continue;
		return j;
	}

	/* No new puzzles? Settle on mastered puzzles, then. */
	if(candidates.length > 0) {
		return parseInt(candidates[Math.floor(Math.random() * candidates.length)], 10);
	}

	return false;
};
