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

const orm_state_version = 1;

const orm_upgrade_state = function(storage) {
	if(typeof(storage) === 'undefined') storage = localStorage;

	let ver = orm_state_get('__orm_version__', -1, storage);

	if(ver > orm_state_version) {
		orm_error('Broken internal state, please reset your progress and refresh the page.');
		return;
	} else if(ver === orm_state_version) {
		return;
	}

	if(ver === -1) {
		ver = 1;

		/* Merge all journal entries into one */
		let journal = [];
		for(let k in storage) {
			if(!k.match(/^journal_/)) continue;
			let j = orm_state_get(k, [], storage);
			let setid = k.substring(8);
			for(let i in j) {
				journal.push([
					j[i][0],
					[ setid, j[i][1][0], j[i][1][1] ]
				]);
			}
			orm_state_unset(k, storage);
		}
		journal.sort(function(a, b) { return b[0] - a[0]; }); /* XXX: more efficient to use orm_journal_merge? */
		if(journal !== []) {
			orm_state_set('journal', journal);
		}
	}

	orm_state_set('__orm_version__', orm_state_version, storage);
};

orm_when_ready.unshift(orm_upgrade_state);
