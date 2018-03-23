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
#include <stdlib.h>
#include <string.h>

void gobble_whitespace(const char** s) {
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

char read_hex_digit(char s) {
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

int read_json_string(char** dest, size_t* destlen, const char* src, size_t srclen) {
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

int find_pgn_tag(const char* src, const char* tag, char* out, size_t outlen) {
	size_t taglen = strlen(tag);

	out[0] = '\0';

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

const char* find_pgn_next_move(const char* pgn, char* out, size_t outlen) {
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
			while(*pgn != '\0' && *pgn != ' ' && *pgn != '\n' && *pgn != '+' && *pgn != '#' && *pgn != '?' && *pgn != '!') {
				if(outlen > 0) {
					--outlen;
					*out = *pgn;
					++out;
				}
				++pgn;
			}
			if(outlen > 0) *out = '\0';
			while(*pgn == '+' || *pgn == '#' || *pgn == '?' || *pgn == '!') ++pgn;
			return pgn;

		default:
			return 0; /* Parse error */
		}
	}

	__builtin_unreachable();
}

void strip_last_fields(char* pgn, char fieldsep, unsigned char strip) {
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
