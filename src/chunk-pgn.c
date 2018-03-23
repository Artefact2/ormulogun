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

#include <stdio.h>
#include <stdbool.h>

#define BUFSZ (1 << 15)

int main(void) {
	bool closed = true;
	unsigned char newlines = 0;
	char ibuf[BUFSZ];
	char obuf[BUFSZ << 1];
	size_t ilen = 0;
	size_t olen = 0;
	size_t i;

	while(!feof(stdin)) {
		ilen = fread(ibuf, 1, sizeof(ibuf), stdin);
		olen = 0;

		if(ilen > 0 && closed) {
			closed = false;
			obuf[olen++] = '"';
		}

		for(i = 0; i < ilen; ++i) {
			char c = ibuf[i];
			if(c == '[' && newlines == 2) {
				obuf[olen++] = '"';
				obuf[olen++] = '\n';
				obuf[olen++] = '"';
			}

			if(c == '\n') {
				++newlines;
			} else {
				newlines = 0;
			}

			switch(c) {
			case '\\': obuf[olen++] = '\\'; obuf[olen++] = '\\'; break;
			case '"': obuf[olen++] = '\\'; obuf[olen++] = '"'; break;
			case '\b': obuf[olen++] = '\\'; obuf[olen++] = 'b'; break;
			case '\f': obuf[olen++] = '\\'; obuf[olen++] = 'f'; break;
			case '\n': obuf[olen++] = '\\'; obuf[olen++] = 'n'; break;
			case '\r': obuf[olen++] = '\\'; obuf[olen++] = 'r'; break;
			case '\t': obuf[olen++] = '\\'; obuf[olen++] = 't'; break;
			default: obuf[olen++] = c;
			}
		}

		obuf[olen++] = '\0';
		fputs(obuf, stdout);
	}

	if(!closed) {
		puts("\"");
	}

	return 0;
}
