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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <gumble.h>
#include <Judy.h>
#include "common.h"

int main(void) {
	char* line = 0;
	char* pgn = 0;
	size_t linelen, pgnlen;

	cch_board_t b;
	cch_move_t m;
	cch_return_t r;
	char fen[SAFE_FEN_LENGTH];
	char san[SAFE_ALG_LENGTH];
	char key[512];
	size_t l;

	bool no_eco, no_opening, no_variation;
	char eco[4];
	char opening[512];
	char variation[512];

	void* main_array = 0; /* opening -> sub array */
	void** sub_array; /* fen -> count */
	unsigned int* sub_array_value;

	while(getline(&line, &linelen, stdin) != -1) {
		if(read_json_string(&pgn, &pgnlen, line, linelen)) {
			fprintf(stderr, "Unreadable PGN line: %s", line);
			continue;
		}

		no_eco = find_pgn_tag(pgn, "ECO", eco, sizeof(eco));
		no_opening = find_pgn_tag(pgn, "Opening", opening, sizeof(opening));
		no_variation = find_pgn_tag(pgn, "Variation", variation, sizeof(variation));
		if(no_eco && no_opening && no_variation) continue;
		if(!no_eco && eco[0] == '?') continue;
		if(!no_opening && opening[0] == '?') continue;

		snprintf(key, sizeof(key), "%s\t%s\t%s", eco, opening, variation);
		JSLI(sub_array, main_array, key);

		cch_init_board(&b);

		const char* next = pgn;
		while((next = find_pgn_next_move(next, san, SAFE_ALG_LENGTH))) {
			r = cch_parse_san_move(&b, san, &m);
			if(r != CCH_OK) {
				fprintf(stderr, "Erroneous move: %s in game %s", san, line);
				break;
			}

			r = cch_play_legal_move(&b, &m, 0);
			assert(r == CCH_OK);

			r = cch_save_fen(&b, fen, SAFE_FEN_LENGTH);
			strip_last_fields(fen, ' ', 4);
			l = strlen(fen);
			snprintf(&(fen[l]), SAFE_FEN_LENGTH - l, "\t%u", 2 * (b.turn - 1) + (1 - b.side));

			JSLI(sub_array_value, *sub_array, fen);
			++*sub_array_value;

			if(b.turn >= TURN_LIMIT) break;
		}
	}

	key[0] = '\0';
	JSLF(sub_array, main_array, key);
	while(sub_array) {
		fen[0] = '\0';
		JSLF(sub_array_value, *sub_array, fen);
		while(sub_array_value) {
			printf("%u\t0\t0\t%s\t%s\n", *sub_array_value, key, fen);
			JSLN(sub_array_value, *sub_array, fen);
		}
		//JLFA(linelen, *sub_array); XXX: fails with free(): invalid pointer
		JSLN(sub_array, main_array, key);
	}

	if(line) free(line);
	if(pgn) free(pgn);
	JLFA(linelen, main_array);
	return 0;
}
