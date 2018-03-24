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

static void insert_result(const char* result, const char* key, void** array, book_entry_t** values, size_t* values_used, size_t* values_count) {
	unsigned int* array_value;

	JSLI(array_value, *array, key);
	if(!*array_value) {
		/* Newly inserted entry, assign an index in values array */
		if(*values_used == *values_count) {
			*values_count *= 1.4;
			*values = realloc(*values, *values_count * sizeof(book_entry_t));
			assert(*values);
		}

		assert(*values_used < *values_count);
		(*values)[*values_used].white = 0;
		(*values)[*values_used].draw = 0;
		(*values)[*values_used].black = 0;
		*array_value = *values_used;
		++*values_used;
	}

	if(result[0] == '0') {
		++(*values)[*array_value].black;
	} else if(result[1] == '/') {
		++(*values)[*array_value].draw;
	} else {
		++(*values)[*array_value].white;
	}
}

int main(void) {
	char* line = 0;
	char* pgn = 0;
	size_t linelen, pgnlen;

	cch_board_t b;
	cch_move_t m;
	cch_return_t r;

	char result[10];
	char key[SAFE_FEN_LENGTH + SAFE_ALG_LENGTH + 1];
	size_t l;

	void* array = 0;
	unsigned int* array_value;
	book_entry_t* values = malloc(100 * sizeof(book_entry_t));
	size_t values_used = 0, values_count = 100;
	assert(values);

	while(getline(&line, &linelen, stdin) != -1) {
		if(read_json_string(&pgn, &pgnlen, line, linelen)) {
			fprintf(stderr, "Unparseable PGN line: %s", line);
			continue;
		}

		if(find_pgn_tag(pgn, "Result", result, 100)) continue;
		if(strcmp("1-0", result) && strcmp("0-1", result) && strcmp("1/2-1/2", result)) continue;

		cch_init_board(&b);

		const char* next = pgn;
		while((next = find_pgn_next_move(next, key, SAFE_ALG_LENGTH))) {
			r = cch_parse_san_move(&b, key, &m);
			if(r != CCH_OK) {
				fprintf(stderr, "Erroneous move: %s in game %s", key, line);
				break;
			}

			r = cch_save_fen(&b, key, SAFE_FEN_LENGTH);
			assert(r == CCH_OK);
			strip_last_fields(key, ' ', 2);

			l = strlen(key);
			key[l] = '\t';
			key[l+1] = '\0';
			insert_result(result, key, &array, &values, &values_used, &values_count);

			r = cch_format_lan_move(&m, key + l + 1, sizeof(key) - l - 1);
			assert(r == CCH_OK);
			insert_result(result, key, &array, &values, &values_used, &values_count);

			if(b.turn >= TURN_LIMIT) break;

			r = cch_play_legal_move(&b, &m, 0);
			assert(r == CCH_OK);
		}
	}

	key[0] = '\0';
	JSLF(array_value, array, key);
	while(array_value) {
		printf(
			"%u\t%u\t%u\t%s\n",
			values[*array_value].white,
			values[*array_value].draw,
			values[*array_value].black,
			key
			);
		JSLN(array_value, array, key);
	}

	if(line) free(line);
	if(pgn) free(pgn);
	if(values) free(values);
	JSLFA(linelen, array);
	return 0;
}
