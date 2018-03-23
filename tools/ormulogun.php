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

assert_options(ASSERT_BAIL, 1);

function orm_parse_pgn(string $pgn): array {
	list($header, $body) = explode("\n\n", $pgn."\n\n", 2);

	preg_match_all('%^\[(?<k>[A-Z][A-Za-z]*) "(?<v>[^"]*)"\]$%m', $header, $match, PREG_SET_ORDER);
	$tags = [];
	foreach($match as $m) {
		$tags[$m['k']] = $m['v'];
	}

	$moves = preg_replace('% \{[^}]*\} %', ' ', $body);
	preg_match_all('%(?<san>(O-O-O|O-O|((([KQRBN][a-h]?[1-8]?)|[a-h])x?)?[a-h][1-8](=[QRBN])?))(\+|#|\?!|\?|\?\?)?%', $moves, $matches);
	$tags['Moves'] = $matches['san'];

	return $tags;
}

function orm_do_san_move(?string $startfen, ?string $san): ?string {
	static $proc = null;
	static $pipes = null;

	if($proc === null) {
		$proc = proc_open(__DIR__.'/../build/gumble/src/gumble', [
			0 => [ 'pipe', 'r' ], 1 => [ 'pipe', 'w' ],
		], $pipes);
	}

	$pos = ($startfen === null) ? 'startpos' : 'fen '.$startfen;

	if($san !== null) {
		fprintf($pipes[0], "position %s\nlan %s\n", $pos, $san);
		fflush($pipes[0]);
		$lan = explode(' ', fgets($pipes[1]));
		if($lan[0] !== 'info' || $lan[1] !== 'lan') {
			/* Illegal move, most likely... */
			@fclose($pipes[0]);
			@fclose($pipes[1]);
			@proc_close($proc);
			$proc = null;
			$pipes = null;
			return null;
		}
		$pos .= ' moves '.trim($lan[2]);
	}

	fprintf($pipes[0], "position %s\nfen\n", $pos);
	fflush($pipes[0]);
	$fen = explode(' ', fgets($pipes[1]), 3);
	assert($fen[0] === 'info' && $fen[1] === 'fen');
	return trim($fen[2]);
}

function orm_gumble(string $cmd, bool $expectresult = true): ?string {
	static $proc = null;
	static $pipes = null;

	if($proc === null) {
		$proc = proc_open(__DIR__.'/../build/gumble/src/gumble', [ 0 => [ 'pipe', 'r' ], 1 => [ 'pipe', 'w' ] ], $pipes);
		assert($proc !== false);
	}

	fwrite($pipes[0], $cmd."\n");
	if($expectresult === false) return null;

	$line = fgets($pipes[1]);
	assert($line !== false);
	$line = explode(' ', substr($line, 0, -1), 3);
	assert($line[0] === 'info' && $line[1] === explode(' ', $cmd, 2)[0]);
	return $line[2];
}

function orm_bookfen(string $fen): string {
	return implode(' ', array_slice(explode(' ', $fen, 5), 0, 4));
}

function orm_ecobookfen(string $fen): string {
	return implode(' ', array_slice(explode(' ', $fen, 3), 0, 2));
}
