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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gumble.h>
#include <Judy.h>
#include "common.h"

typedef struct {
	unsigned int white;
	unsigned int draw;
	unsigned int black;
} book_entry_t;

int main(void) {
	char* line = 0;
	char* pgn = 0;
	size_t linelen, pgnlen;

	cch_board_t b;
	cch_move_t m;
	cch_return_t r;
	char fen[SAFE_FEN_LENGTH];
	char san[SAFE_ALG_LENGTH];

	void* array = 0;
	unsigned int* array_value;
	unsigned int* root_value = 0;
	book_entry_t* values = malloc(100 * sizeof(book_entry_t));
	size_t values_used = 0, values_count = 100;
	assert(values);

	while(getline(&line, &linelen, stdin) != -1) {
		if(read_json_string(&pgn, &pgnlen, line, linelen)) {
			fprintf(stderr, "Unparseable PGN line: %s", line);
			continue;
		}

		char result[10];
		if(find_pgn_tag(pgn, "Result", result, 100)) continue;
		if(strcmp("1-0", result) && strcmp("0-1", result) && strcmp("1/2-1/2", result)) continue;

		cch_init_board(&b);

		if(root_value == 0) {
			r = cch_save_fen(&b, fen, SAFE_FEN_LENGTH);
			assert(r == CCH_OK);
			strip_last_fields(fen, ' ', 2);
			JSLI(root_value, array, fen);
			*root_value = 0;
			values[0].white = 0;
			values[0].draw = 0;
			values[0].black = 0;
		}

		if(result[0] == '0') {
			++values[0].black;
		} else if(result[1] == '/') {
			++values[0].draw;
		} else {
			++values[0].white;
		}

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
			assert(r == CCH_OK);
			strip_last_fields(fen, ' ', 2);

			JSLI(array_value, array, fen);
			if(!*array_value) {
				/* Newly inserted entry, assign an index in values array */
				if(values_used == values_count) {
					values_count *= 1.4;
					values = realloc(values, values_count * sizeof(book_entry_t));
					assert(values);
				}

				assert(values_used < values_count);
				values[values_used].white = 0;
				values[values_used].draw = 0;
				values[values_used].black = 0;
				*array_value = values_used;
				++values_used;
			}

			if(result[0] == '0') {
				++values[*array_value].black;
			} else if(result[1] == '/') {
				++values[*array_value].draw;
			} else {
				++values[*array_value].white;
			}

			if(b.turn >= TURN_LIMIT) break;
		}
	}

	fen[0] = '\0';
	JSLF(array_value, array, fen);
	while(array_value) {
		printf(
			"%u\t%u\t%u\t%s\n",
			values[*array_value].white,
			values[*array_value].draw,
			values[*array_value].black,
			fen
			);
		JSLN(array_value, array, fen);
	}

	if(line) free(line);
	if(pgn) free(pgn);
	if(values) free(values);
	JSLFA(linelen, array);
	return 0;
}
