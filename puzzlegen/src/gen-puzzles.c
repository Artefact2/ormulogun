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

#include "gen-puzzles.h"
#include "config.h"
#include <engine.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void usage(char* me) {
	fprintf(stderr,
			"Usage: %s [--verbose] [--start-ply N] [--max-puzzles N]\n"
			"          [--uci-engine-limiter-probe foo] [--uci-engine-limiter foo]\n"
			"          [--max-depth N] [--eval-cutoff N] [--max-variations N] [--variation-eval-cutoff N]\n"
			"          [--best-eval-cutoff-start N] [--best-eval-cutoff-continue N]\n"
			"          <games...>\n"
			"See config.h for an explanation of these options.\n"
			"Where each game is a JSON array of SAN moves from starting position.\n"
			"Example: %s '[\"e4\",\"Nc6\"]'\n", me, me);
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

static const char* expect_string(int argc, char** argv) {
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
	unsigned int max_puzzles = 0;
	bool verbose = false;

	if(argc == 1) {
		usage(*argv);
	}

	--argc;
	++argv;
	while(argc > 0) {
		if(!strcmp("--verbose", *argv)) {
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
		} else if(!strcmp("--eval-cutoff", *argv)) {
			--argc; ++argv;
			s.eval_cutoff = expect_number(argc, argv);
		} else if(!strcmp("--max-variations", *argv)) {
			--argc; ++argv;
			s.max_variations = expect_number(argc, argv);
		} else if(!strcmp("--variation-eval-cutoff", *argv)) {
			--argc; ++argv;
			s.variation_eval_cutoff = expect_number(argc, argv);
		} else if(!strcmp("--best-eval-cutoff-start", *argv)) {
			--argc; ++argv;
			s.best_eval_cutoff_start = expect_number(argc, argv);
		} else if(!strcmp("--best-eval-cutoff-continue", *argv)) {
			--argc; ++argv;
			s.best_eval_cutoff_continue = expect_number(argc, argv);
		}

		else if(!strncmp("--", *argv, 2)) {
			fprintf(stderr, "Unknown argument %s\n", *argv);
			exit(1);
		} else break;

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
	puzzle_t p;
	puzzle_step_t* st = 0;
	bool strep = false; /* false -> st.next[?].move, true -> st.reply */
	unsigned int puzzles = 0, ply;

	char lanmove[SAFE_ALG_LENGTH];
	char* sanmove;
	char* saveptr;

	p.root.next = 0;

	for(int i = 0; i < argc; ++i) {
		cch_init_board(&b);
		fputs("ucinewgame\n", ctx.w);

		for(sanmove = strtok_r(argv[i], "[]\",", &saveptr), ply = 1; sanmove; sanmove = strtok_r(0, "[]\",", &saveptr), ++ply) {
			if(verbose) fprintf(stderr, "move %s\n", sanmove);

			puzzle_init(&p, &b);
			ret = cch_parse_san_move(&b, sanmove, &m);
			assert(ret == CCH_OK);
			ret = cch_play_move(&b, &m, 0);
			assert(ret == CCH_OK);
			ret = cch_format_lan_move(&m, lanmove, SAFE_ALG_LENGTH);
			assert(ret == CCH_OK);

			if(ply < s.min_ply) continue;

			if(st) {
				if(strep) {
					if(!strcmp(lanmove, st->reply)) {
						strep = false;
						continue;
					}
					st = 0;
				} else {
					for(unsigned char j = 0; j < st->nextlen; ++j) {
						if(!strcmp(lanmove, st->next[j].move)) {
							st = &(st->next[j]);
							strep = true;
							break;
						}
					}
					if(strep) continue;
					else st = 0;
				}
			}

			nlines = uci_eval(&ctx, limiterp, &b, evals, s.max_variations + 1);
			if(!puzzle_consider(evals, nlines, s, 0)) continue;

			if(p.root.next) puzzle_free(&p);
			strncpy(p.root.reply, lanmove, SAFE_ALG_LENGTH);
			puzzle_build(&ctx, &p, &b, limiter, s);
			if(p.min_depth > 0) {
				++puzzles;
				puzzle_print(&p);
				st = &(p.root);
				strep = false;
			} else {
				st = 0;
			}

			if(puzzles > 0 && puzzles == max_puzzles) {
				i = argc;
				break;
			}
		}

		if(verbose) fputs("game\n", stderr);
	}

	uci_quit(&ctx);
	return 0;
}
