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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct {
	unsigned int w;
	unsigned int d;
	unsigned int l;
	char payload[1024];
} book_entry_t;

int main(int argc, char** argv) {
	if(argc < 3) {
		fprintf(stderr, "Usage: %s <prune-threshold> book1.tsv book2.tsv... > out.tsv\nAll books are assumed in ascending order.\n", argv[0]);
		return 1;
	}

	unsigned int pthresh = strtoul(argv[1], 0, 10);
	char min[1024];
	int i;

	argc -= 2;
	argv += 2;

	FILE* in[argc];
	book_entry_t lines[argc];

	memset(lines, 0, sizeof(lines));

	for(i = 0; i < argc; ++i) {
		in[i] = fopen(argv[i], "rb");
		assert(in[i]);
	}

	while(argc > 0) {
		min[0] = 0;

		/* Read new lines and find the minimum */
		for(i = 0; i < argc; ++i) {
			if(lines[i].payload[0] == 0) {
				/* XXX: this is extremely fragile */
				if(fscanf(in[i], "%u\t%u\t%u\t%1023[^\n]", &lines[i].w, &lines[i].d, &lines[i].l, lines[i].payload) == EOF) {
					fclose(in[i]);
					in[i] = in[argc - 1];
					lines[i] = lines[argc - 1];
					argv[i] = argv[argc - 1];
					--i;
					--argc;
					continue;
				}
			}

			assert(lines[i].payload[0]);

			if(min[0] == 0 || strcmp(lines[i].payload, min) < 0) {
				strcpy(min, lines[i].payload);
			}
		}

		if(argc == 0) break;
		assert(min[0]);

		/* Merge minimums and print it */
		unsigned int w = 0, d = 0, l = 0;
		for(i = 0; i < argc; ++i) {
			int c = strcmp(min, lines[i].payload);
			assert(c <= 0);
			if(c) continue;

			w += lines[i].w;
			d += lines[i].d;
			l += lines[i].l;
			lines[i].payload[0] = 0;
		}

		if(w + d + l >= pthresh) {
			fprintf(stdout, "%u\t%u\t%u\t%s\n", w, d, l, min);
		}
	}

	return 0;
}
