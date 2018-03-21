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

#include "ormulogun.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void usage(void) {
	fprintf(stderr,
			"Usage: gen-puzzles [--verbose] [--start-fen fen] [--start-ply N] [--max-puzzles N]\n"
			"                   [--uci-engine-limiter-probe foo] [--uci-engine-limiter foo]\n"
			"                   [--max-depth N] [--max-variations N]\n"
			"                   [--min-eval-cutoff N] [--max-eval-cutoff N]\n"
			"                   [--puzzle-threshold-absolute N] [--variation-cutoff-relative D]\n"
			"                   < json-movelists...\n"
			"See config.h for an explanation of these options.\n"
			"Where each game is a JSON array of SAN moves from starting position.\n"
			"Environment toggles: ORM_VERBOSE_EVAL\n"
			"Example: echo '[\"e4\",\"Nc6\"]' | gen-puzzles\n");
	exit(1);
}

static int expect_number(int argc, char** argv) {
	if(argc >= 1) {
		char* end;
		int num = strtol(*argv, &end, 10);
		if(**argv != '\0' && *end == '\0') return num;
	}

	fprintf(stderr, "Expected a numeric argument after %s\n", argv[-1]);
	exit(1);
}

static float expect_float(int argc, char** argv) {
	if(argc >= 1) {
		char* end;
		float num = strtof(*argv, &end);
		if(**argv != '\0' && *end == '\0') return num;
	}

	fprintf(stderr, "Expected a numeric argument after %s\n", argv[-1]);
	exit(1);
}

static char* expect_string(int argc, char** argv) {
	if(argc >= 1) {
		return *argv;
	}

	fprintf(stderr, "Expected a string argument after %s\n", argv[-1]);
	exit(1);
}

int main(int argc, char** argv) {
	puzzlegen_settings_t s = settings;
	const char* limiterp = uci_limiter_probe;
	const char* limiter = uci_limiter;
	const char* startfen = 0;
	unsigned int max_puzzles = 0;
	bool verbose = false;

	--argc;
	++argv;
	while(argc > 0) {
		if(!strcmp("--help", *argv) || !strcmp("-h", *argv)) {
			usage();
		} else if(!strcmp("--verbose", *argv)) {
			verbose = true;
		} else if(!strcmp("--start-ply", *argv)) {
			--argc; ++argv;
			s.min_ply = expect_number(argc, argv);
		} else if(!strcmp("--max-puzzles", *argv)) {
			--argc; ++argv;
			max_puzzles = expect_number(argc, argv);
		} else if(!strcmp("--uci-engine-limiter-probe", *argv)) {
			--argc; ++argv;
			limiterp = expect_string(argc, argv);
		} else if(!strcmp("--uci-engine-limiter", *argv)) {
			--argc; ++argv;
			limiter = expect_string(argc, argv);
		} else if(!strcmp("--max-depth", *argv)) {
			--argc; ++argv;
			s.max_depth = expect_number(argc, argv);
		} else if(!strcmp("--max-variations", *argv)) {
			--argc; ++argv;
			s.max_variations = expect_number(argc, argv);
		} else if(!strcmp("--min-eval-cutoff", *argv)) {
			--argc; ++argv;
			s.min_eval_cutoff = expect_number(argc, argv);
		} else if(!strcmp("--max-eval-cutoff", *argv)) {
			--argc; ++argv;
			s.max_eval_cutoff = expect_number(argc, argv);
		} else if(!strcmp("--puzzle-threshold-absolute", *argv)) {
			--argc; ++argv;
			s.puzzle_threshold_absolute = expect_number(argc, argv);
		} else if(!strcmp("--variation-cutoff-relative", *argv)) {
			--argc; ++argv;
			s.variation_cutoff_relative = expect_float(argc, argv);
		} else if(!strcmp("--start-fen", *argv)) {
			--argc; ++argv;
			startfen = expect_string(argc, argv);
		} else {
			fprintf(stderr, "Unknown argument %s\n", *argv);
			exit(1);
		}

		--argc; ++argv;
	}

	uci_engine_context_t ctx;
	if(uci_create(uci_engine, &ctx)) {
		perror("could not spawn engine");
		return 1;
	}
	uci_init(&ctx, uci_engine_opts);

	cch_board_t b;
	cch_move_t m;
	cch_return_t ret;

	uci_eval_t evals[s.max_variations + 1];
	unsigned char nlines;
	unsigned int npuzzles = 0, ply;

	char* movestr;
	char* saveptr;

	/* Keep the last two generated puzzles, one for each side. This
	 * prevents generating puzzles that are children of the previously
	 * generated puzzle for that side. */
	struct {
		puzzle_t p;
		puzzle_step_t* st;
	} puzzles[2];
	puzzle_t* p;

	memset(puzzles, 0, sizeof(puzzles));

	char* line = 0;
	size_t linelen;

	while(getline(&line, &linelen, stdin) != -1) {
		if(startfen) {
			ret = cch_load_fen(&b, startfen);
			assert(ret == CCH_OK);
		} else {
			cch_init_board(&b);
		}
		fputs("ucinewgame\n", ctx.w);

		for(movestr = strtok_r(line, "[]\",\n", &saveptr), ply = 1; movestr; movestr = strtok_r(0, "[]\",\n", &saveptr), ++ply) {
			if(verbose) fprintf(stderr, "move %s\n", movestr);

			p = &(puzzles[!b.side].p);

			ret = cch_parse_lan_move(movestr, &m);
			if(ret == CCH_PARSE_ERROR) {
				ret = cch_parse_san_move(&b, movestr, &m);
			}
			assert(ret == CCH_OK);
			puzzle_init(p, &b, &m);
			ret = cch_play_move(&b, &m, 0);
			assert(ret == CCH_OK);

			if(ply < s.min_ply) continue;

			if(puzzles[!b.side].st) {
				bool found = false;
				for(unsigned char j = 0; j < puzzles[!b.side].st->nextlen; ++j) {
					if(ORM_MOVES_EQUAL(m, puzzles[!b.side].st->next[j].move)) {
						found = true;
						puzzles[!b.side].st = &(puzzles[!b.side].st->next[j]);
						break;
					}
				}
				if(!found) {
					puzzles[!b.side].st = 0;
				}
			}
			if(puzzles[b.side].st) {
				if(!ORM_MOVES_EQUAL(m, puzzles[b.side].st->reply)) {
					puzzles[b.side].st = 0;
				} else {
					continue;
				}
			}

			nlines = uci_eval(&ctx, limiterp, &b, evals, s.max_variations + 1);
			nlines = puzzle_consider(evals, nlines, s, 0);
			if(nlines == 0 || nlines == 255) continue;

			if(p->root.next) puzzle_free(p);
			puzzle_build(&ctx, p, &b, limiter, s);
			if(p->min_depth > 0) {
				++npuzzles;
				puzzle_print(p);
				puzzles[b.side].st = &(p->root);
			} else {
				puzzles[b.side].st = 0;
			}

			if(npuzzles > 0 && npuzzles == max_puzzles) {
				break;
			}
		}

		if(verbose) fputs("game\n", stderr);
	}

	free(line);
	uci_quit(&ctx);
	return 0;
}
