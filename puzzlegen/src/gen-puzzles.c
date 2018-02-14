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
	fprintf(stderr, "Usage: %s [--start-ply N] [--max-puzzles N] <games...>\n"
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

int main(int argc, char** argv) {
	puzzlegen_settings_t s = settings;
	unsigned int max_puzzles = 0;

	if(argc == 1) {
		usage(*argv);
	}

	--argc;
	++argv;
	while(argc > 0) {
		if(!strcmp("--start-ply", *argv)) {
			--argc;
			++argv;
			s.min_ply = expect_number(argc, argv);
		} else if(!strcmp("--max-puzzles", *argv)) {
			--argc;
			++argv;
			max_puzzles = expect_number(argc, argv);
		} else if(!strncmp("--", *argv, 2)) {
			fprintf(stderr, "Unknown argument %s\n", *argv);
			exit(1);
		} else break;

		--argc;
		++argv;
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
	unsigned int puzzles = 0, ply;

	char lanlist[4096]; /* XXX */
	size_t lanlistlen;
	char* sanmove;
	char lanmove[SAFE_ALG_LENGTH];
	char* saveptr;

	for(int i = 0; i < argc; ++i) {
		cch_init_board(&b);
		fputs("ucinewgame\n", ctx.w);
		lanlistlen = 0;

		for(sanmove = strtok_r(argv[i], "[]\",", &saveptr), ply = 1; sanmove; sanmove = strtok_r(0, "[]\",", &saveptr), ++ply) {
			puzzle_init(&p, &b);
			ret = cch_parse_san_move(&b, sanmove, &m);
			assert(ret == CCH_OK);
			ret = cch_format_lan_move(&m, lanmove, SAFE_ALG_LENGTH);
			assert(ret == CCH_OK);
			ret = cch_play_move(&b, &m, 0);
			assert(ret == CCH_OK);

			if(lanlistlen) {
				lanlist[lanlistlen] = ' ';
				++lanlistlen;
			}
			strncpy(lanlist + lanlistlen, lanmove, SAFE_ALG_LENGTH);
			lanlistlen += strlen(lanmove);

			if(ply < s.min_ply) continue;

			nlines = uci_eval(&ctx, uci_limiter_probe, lanlist, evals, s.max_variations + 1);
			if(!puzzle_consider(evals, nlines, s, 0)) continue;

			strncpy(p.root.reply, lanmove, SAFE_ALG_LENGTH);
			puzzle_build(&ctx, lanlist, lanlistlen, &p, &b, uci_limiter, s);
			if(p.min_depth > 0) {
				++puzzles;
				puzzle_print(&p);
			}
			puzzle_free(&p);

			if(puzzles > 0 && puzzles == max_puzzles) {
				i = argc;
				break;
			}
		}
	}

	uci_quit(&ctx);
	return 0;
}
