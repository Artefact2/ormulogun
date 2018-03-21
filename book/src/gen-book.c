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

typedef struct {
	unsigned int white;
	unsigned int draw;
	unsigned int black;
} book_entry_t;

static void gobble_whitespace(const char** s) {
	while(1) {
		switch(**s) {
		case '\t':
		case '\n':
		case '\r':
		case ' ':
			++*s;
			break;

		default: return;
		}
	}
}

static char read_hex_digit(char s) {
	switch(s) {
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'a': case 'A': return 10;
	case 'b': case 'B': return 11;
	case 'c': case 'C': return 12;
	case 'd': case 'D': return 13;
	case 'e': case 'E': return 14;
	case 'f': case 'F': return 15;
	}
	return -1;
}

static int read_json_string(char** dest, size_t* destlen, const char* src, size_t srclen) {
	if(*dest == 0 || srclen > *destlen) {
		*dest = realloc(*dest, srclen);
		assert(*dest);
		*destlen = srclen;
	}

	char* out = *dest;

	gobble_whitespace(&src);

	if(*src != '"') return 1;
	++src;

	while(*src) {
		switch(*src) {
		case '"':
			*out = '\0';
			return 0;

		case '\\':
			++src;
			switch(*src) {
			case '"':
			case '\\':
			case '/':
				*out = *src; break;

			case 'b': *out = '\b'; break;
			case 'f': *out = '\f'; break;
			case 'n': *out = '\n'; break;
			case 'r': *out = '\r'; break;
			case 't': *out = '\t'; break;

			case 'u':
				++src;
				if(src[0] == '\0' || src[1] == '\0' || src[2] == '\0' || src[3] == '\0') return 1;
				char a = read_hex_digit(src[0]),
					b = read_hex_digit(src[1]),
					c = read_hex_digit(src[2]),
					d = read_hex_digit(src[3]);
				if(a == -1 || b == -1 || c == -1 || d == -1) return 1;
				unsigned short cp = (a << 12) | (b << 8) | (c << 4) | d;
				/* Assume UTF-8 output */
				if(cp < 0x80) *out = cp;
				else if(cp < 0x800) {
					out[0] = 192 | (cp >> 6);
					out[1] = 128 | (cp & 63);
					out += 1;
				} else {
					out[0] = 224 | (cp >> 12);
					out[1] = 128 | ((cp >> 6) & 63);
					out[2] = 128 | (cp & 63);
					out += 2;
				}
				src += 3;
				break;

			default: return 1; /* Invalid escape sequence */
			}
			++out;
			++src;
			break;

		default:
			*out = *src;
			++out;
			++src;
		}
	}

	/* No end quote found? */
	return 1;
}

static int find_pgn_tag(const char* src, const char* tag, char* out, size_t outlen) {
	size_t taglen = strlen(tag);

	while(*src) {
		if(src[0] == '\n' && src[1] == '\n') return 1; /* End of PGN header */
		if(*src != '[') {
			/* Not an opening tag */
			++src;
			continue;
		}

		if(strncmp(src + 1, tag, taglen)) {
			/* Not the correct tag */
			++src; /* XXX: inefficient, but no assumptions about length of src are made */
			continue;
		}

		src += 1 + taglen;
		if(src[0] != ' ' || src[1] != '"') {
			++src;
			continue;
		}

		src += 2;
		while(*src) {
			if(src[0] == '"' && src[1] == ']' && src[2] == '\n') {
				/* End of tag */
				if(outlen == 0) return 2; /* Not enough space */
				*out = '\0';
				return 0;
			}

			if(src[0] == '\n' || src[0] == '\0') {
				/* Tag not properly closed */
				return 1;
			}

			if(outlen == 0) return 2; /* Not enough space */
			*out = *src;
			++out;
			++src;
			--outlen;
		}
	}

	return 1;
}

static const char* find_pgn_next_move(const char* pgn, char* out, size_t outlen) {
	while(1) {
		switch(*pgn) {
		case '\0':
			/* End of PGN, no more move to find */
			return 0;

		case '[':
			/* PGN tag, gobble it all */
			while(1) {
				++pgn;
				if(*pgn == '\0') return 0;
				if(*pgn == ']') break;
			}
			++pgn;
			break;


		case '{':
			/* PGN annotation, gobble it all */
			while(1) {
				++pgn;
				if(*pgn == '\0') return 0;
				if(*pgn == '}') break;
			}
			++pgn;
			break;

		case ' ':
		case '\n':
			/* Eat whitespace */
			++pgn;
			break;

		case '0':
		case '1':
			/* Could be end of game (1-0, etc.) or a turn count (123. ...) */
			if(!strcmp(pgn, "1-0") || !strcmp(pgn, "0-1") || !strcmp(pgn, "1/2-1/2")) {
				/* Game over, no more moves */
				return 0;
			}
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			/* Turn counter, expect a sequence of digits followed by dots */
			while(*pgn >= '0' && *pgn <= '9') ++pgn;
			while(*pgn == '.') ++pgn;
			break;

		case 'K':
		case 'Q':
		case 'R':
		case 'B':
		case 'N':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'O':
			/* Looks like the start of a move */
			while(1) {
				if(outlen > 0) {
					--outlen;
					*out = *pgn;
					++out;
				}
				++pgn;
				if(*pgn == '\0') {
					if(outlen > 0) *out = '\0';
					return pgn;
				}
				if(*pgn == ' ' || *pgn == '\n' || *pgn == '+' || *pgn == '#') {
					if(outlen > 0) *out = '\0';
					return pgn + 1;
				}
			}
			break;

		default:
			return 0; /* Parse error */
		}
	}

	__builtin_unreachable();
}

static void strip_last_fields(char* pgn, char fieldsep, unsigned char strip) {
	char* cur = pgn;
	while(*cur) ++cur;
	while(strip && cur > pgn) {
		if(cur[-1] == fieldsep) {
			--strip;
			if(strip == 0) {
				cur[-1] = '\0';
				return;
			}
		}
		--cur;
	}
}

int main(int argc, char** argv) {
	char* line = 0;
	char* pgn = 0;
	size_t linelen, pgnlen;

	cch_board_t b;
	cch_move_t m;
	cch_return_t r;
	char fen[SAFE_FEN_LENGTH];
	char san[SAFE_ALG_LENGTH];

	void* array = 0;
	unsigned int* array_value;
	unsigned int* root_value = 0;
	book_entry_t* values = malloc(100 * sizeof(book_entry_t));
	size_t values_used = 0, values_count = 100;
	assert(values);

	while(getline(&line, &linelen, stdin) != -1) {
		if(read_json_string(&pgn, &pgnlen, line, linelen)) {
			fprintf(stderr, "Unparseable PGN line: %s", line);
			continue;
		}

		char result[10];
		if(find_pgn_tag(pgn, "Result", result, 100)) continue;
		if(strcmp("1-0", result) && strcmp("0-1", result) && strcmp("1/2-1/2", result)) continue;

		cch_init_board(&b);

		if(root_value == 0) {
			r = cch_save_fen(&b, fen, SAFE_FEN_LENGTH);
			assert(r == CCH_OK);
			strip_last_fields(fen, ' ', 2);
			JSLI(root_value, array, fen);
			*root_value = 0;
			values[0].white = 0;
			values[0].draw = 0;
			values[0].black = 0;
		}

		if(result[0] == '0') {
			++values[0].black;
		} else if(result[1] == '/') {
			++values[0].draw;
		} else {
			++values[0].white;
		}

		const char* next = pgn;
		while((next = find_pgn_next_move(next, san, SAFE_ALG_LENGTH))) {
			r = cch_parse_san_move(&b, san, &m);
			if(r != CCH_OK) {
				fprintf(stderr, "Erroneous move: %s in game %s", san, line);
				break;
			}

			r = cch_play_legal_move(&b, &m, 0);
			assert(r == CCH_OK);

			r = cch_save_fen(&b, fen, SAFE_FEN_LENGTH);
			assert(r == CCH_OK);
			strip_last_fields(fen, ' ', 2);

			JSLI(array_value, array, fen);
			if(!*array_value) {
				/* Newly inserted entry, assign an index in values array */
				if(values_used == values_count) {
					values_count *= 1.4;
					values = realloc(values, values_count * sizeof(book_entry_t));
					assert(values);
				}

				assert(values_used < values_count);
				values[values_used].white = 0;
				values[values_used].draw = 0;
				values[values_used].black = 0;
				*array_value = values_used;
				++values_used;
			}

			if(result[0] == '0') {
				++values[*array_value].black;
			} else if(result[1] == '/') {
				++values[*array_value].draw;
			} else {
				++values[*array_value].white;
			}
		}
	}

	fen[0] = '\0';
	JSLF(array_value, array, fen);
	while(array_value) {
		printf(
			"%u\t%u\t%u\t%s\n",
			values[*array_value].white,
			values[*array_value].draw,
			values[*array_value].black,
			fen
			);
		JSLN(array_value, array, fen);
	}

	if(line) free(line);
	if(pgn) free(pgn);
	if(values) free(values);
	JSLFA(linelen, array);
}
