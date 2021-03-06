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

const orm_state_get = function(k, def, storage) {
	if(typeof(storage) === 'undefined') storage = localStorage;
	if(k in storage) {
		return JSON.parse(storage[k]);
	} else {
		return def;
	}
};

const orm_state_set = function(k, v, storage) {
	if(typeof(storage) === 'undefined') storage = localStorage;
	return storage[k] = JSON.stringify(v);
};

const orm_state_unset = function(k, storage) {
	if(typeof(storage) === 'undefined') storage = localStorage;
	return delete storage[k];
};
