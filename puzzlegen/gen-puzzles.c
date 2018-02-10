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
#include <string.h>
#include <assert.h>

int main(int argc, char** argv) {
	cch_board_t b;
	cch_move_t m;
	cch_return_t ret;
	char lanlist[4096]; /* XXX */
	size_t lanlistlen;
	uci_engine_context_t ctx;
	char* sanmove;
	char lanmove[SAFE_ALG_LENGTH];
	char* saveptr;
	uci_eval_t evals[10];

	if(argc == 1) {
		fprintf(stderr, "Usage: %s <games...>\n"
				"Where each game is a JSON array of SAN moves from starting position.\n"
				"Example: %s '[\"e4\",\"Nc6\"]'\n", argv[0], argv[0]);
		return 1;
	}

	if(uci_create(uci_engine, &ctx)) {
		perror("could not spawn engine");
		return 1;
	}

	uci_init(&ctx, uci_engine_opts);

	for(int i = 1; i < argc; ++i) {
		cch_init_board(&b);
		fputs("ucinewgame\n", ctx.w);
		lanlistlen = 0;

		for(sanmove = strtok_r(argv[i], "[]\",", &saveptr); sanmove; sanmove = strtok_r(0, "[]\",", &saveptr)) {
			printf("san: %s\n", sanmove);
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
			strcpy(lanlist + lanlistlen, lanmove);
			lanlistlen += strlen(lanmove);

			puts(lanlist);

			unsigned char stop = uci_eval(&ctx, "movetime 1000", lanlist, 10, evals);
			for(unsigned char i = 0; i < stop; ++i) {
				printf("%02d: score %s %d, bestmove %s\n",
					   i, evals[i].type == SCORE_CP ? "cp" : "mate",
					   evals[i].score,
					   evals[i].bestlan);
			}
		}
	}

	uci_quit(&ctx);
	return 0;
}
