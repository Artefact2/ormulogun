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

int main(int argc, char** argv) {
	if(argc == 1) {
		fprintf(stderr, "Usage: %s <puzzles...>\n", argv[0]);
		return 1;
	}

	uci_engine_context_t ctx;
	if(uci_create(uci_engine, &ctx)) {
		perror("could not spawn engine");
		return 1;
	}
	uci_init(&ctx, uci_engine_opts);

	cch_board_t b;
	puzzle_t p;

	for(size_t i = 1; i < argc; ++i) {
		if(puzzle_load(&p, argv[i])) {
			fprintf(stderr, "Parse error loading puzzle: %s\n", argv[i]);
			return 2;
		}

		cch_load_fen(&b, p.fen);
		puzzle_init(&p, &b, &(p.root.reply));
		cch_play_move(&b, &(p.root.reply), 0);
		tags_puzzle(&p, &b, &ctx);
		puzzle_print(&p);
		puzzle_free(&p);
	}

	return 0;
}
