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

		/* XXX: merge all journal entries into one */
	}

	orm_state_set('__orm_version__', orm_state_version, storage);
};

orm_when_ready.unshift(orm_upgrade_state);
