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
});
