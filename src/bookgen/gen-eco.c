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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <gumble.h>
#include <Judy.h>
#include "common.h"

static FILE* zstdpipe(const char* out) {
	int fd[2]; /* read, write */
	int r;

	r = pipe(fd);
	assert(!r);

	switch(fork()) {
	case -1:
		assert(0);
		__builtin_unreachable();

	case 0: /* Child */
		close(fd[1]);
		r = dup2(fd[0], 0); /* set new stdin */
		assert(!r);
		execlp("zstd", "zstd", "-", "-qo", out, 0);
		assert(0);
		__builtin_unreachable();

	default: /* Parent */
		close(fd[0]);
		return fdopen(fd[1], "wb");
	}

	assert(0);
	__builtin_unreachable();
}

int main(int argc, char** argv) {
	if(argc != 2) {
		fprintf(stderr, "Usage: chunk-pgn < input.pgn | %s <out-dir>\n", argv[0]);
		return 1;
	}
	if(chdir(argv[1])) {
		fprintf(stderr, "%s: could not chdir into %s\n", argv[0], argv[1]);
		return 1;
	}

	char* line = 0;
	char* pgn = 0;
	size_t linelen, pgnlen;

	cch_board_t b;
	cch_move_t m;
	cch_return_t r;
	char fen[SAFE_FEN_LENGTH];
	char san[SAFE_ALG_LENGTH];
	char key[512];

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

			b.smoves = 0; /* XXX: for fen "canonicalization" */
			r = cch_save_fen(&b, fen, SAFE_FEN_LENGTH);

			JSLI(sub_array_value, *sub_array, fen);
			++*sub_array_value;

			if(b.turn >= TURN_LIMIT) break;
		}
	}

	FILE* out;
	char outfn[20];
	struct timespec t;
	r = clock_gettime(CLOCK_REALTIME, &t);
	assert(!r);
	snprintf(outfn, 40, "%05d.%ld.%09ld.tsv.zst", getpid(), t.tv_sec, t.tv_nsec);
	key[0] = '\0';
	JSLF(sub_array, main_array, key);
	while(sub_array) {
		r = mkdir(key, 0777);
		assert(!r || errno == EEXIST);
		r = chdir(key);
		assert(!r);
		out = zstdpipe(outfn);
		assert(out);

		fen[0] = '\0';
		JSLF(sub_array_value, *sub_array, fen);
		while(sub_array_value) {
			fprintf(out, "%u\t0\t0\t%s\n", *sub_array_value, fen);
			JSLN(sub_array_value, *sub_array, fen);
		}
		//JLFA(linelen, *sub_array); XXX: fails with free(): invalid pointer
		JSLN(sub_array, main_array, key);

		fclose(out);
		wait(0); /* for zstd to terminate */
		r = chdir("..");
		assert(!r);
	}

	if(line) free(line);
	if(pgn) free(pgn);
	JLFA(linelen, main_array);
	return 0;
}
