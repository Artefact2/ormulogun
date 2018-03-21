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
#include <stdio.h>

int main(void) {
	cch_board_t b;
	puzzle_t p;

	char* line = 0;
	size_t linelen;

	while(getline(&line, &linelen, stdin) != -1) {
		if(puzzle_load(&p, line)) {
			fprintf(stderr, "Parse error loading puzzle: %s", line);
			return 2;
		}

		cch_load_fen(&b, p.fen);
		puzzle_init(&p, &b, &(p.root.reply));
		cch_play_legal_move(&b, &(p.root.reply), 0);
		puzzle_finalize(&p, &b);
		if(p.min_depth > 0) {
			puzzle_print(&p);
		}
		puzzle_free(&p);
	}

	return 0;
}
