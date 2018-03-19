<?php
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

function orm_parse_pgn(string $pgn): array {
	list($header, $body) = explode("\n\n", $pgn, 2);

	preg_match_all('%^\[(?<k>[A-Z][A-Za-z]*) "(?<v>[^"]*)"\]$%m', $header, $match, PREG_SET_ORDER);
	$tags = [];
	foreach($match as $m) {
		$tags[$m['k']] = $m['v'];
	}

	$moves = preg_replace('% \{[^}]*\} %', ' ', $body);
	preg_match_all('%(?<san>(O-O-O|O-O|((([KQRBN][a-h1-8]?)|[a-h])x?)?[a-h][1-8](=[QRBN])?))(\+|#|\?!|\?|\?\?)?%', $moves, $matches);
	$tags['Moves'] = $matches['san'];

	return $tags;
}
