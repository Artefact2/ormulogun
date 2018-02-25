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
#include <string.h>
#include <stdlib.h>

#define EXPECT(call) do {						\
		int ret = (call);						\
		if(ret) return ret;						\
	} while(0)

static void json_whitespace(const char* json, const char** after) {
	while(1) {
		switch(*json) {
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			++json;
			continue;

		default:
			*after = json;
			return;
		}
	}
}

static int json_token(const char* json, char tok, const char** after) {
	json_whitespace(json, &json);
	if(*json == tok) {
		*after = json + 1;
		return 0;
	}
	fprintf(stderr, "Parse error: expecting token %c near %s\n", tok, json);
	return 1;
}

static int json_str(char* dest, const char* json, size_t len, const char** after) {
	EXPECT(json_token(json, '"', &json));

	while(len > 0) {
		switch(*json) {
		case '\\':
			++json;
			switch(*json) {
			case '/':
				*dest = '/';
				++dest;
				++json;
				--len;
				break;

			default:
				fprintf(stderr, "Unsupported escape character near %s\n", json);
				return 1;
			}
			break;

		case '"':
			*dest = '\0';
			++dest;
			++json;
			len = 0;
			break;

		default:
			*dest = *json;
			++dest;
			++json;
			--len;
		}
	}

	if(dest[-1] != '\0') {
		fprintf(stderr, "String overflow near %s\n", json);
		return 1;
	}

	*after = json;
	return 0;
}

static int json_lan(cch_move_t* m, const char* json, const char** after) {
	char lan[SAFE_ALG_LENGTH];
	EXPECT(json_str(lan, json, SAFE_ALG_LENGTH, after));
	return cch_parse_lan_move(lan, m);
}

static int json_puzzle_step(puzzle_step_t* st, const char* json, const char** after) {
	json_whitespace(json, &json);

	if(*json == '\"') {
		/* End of puzzle, read reply */
		st->next = 0;
		st->nextlen = 0;

		if(json[1] == '\"') {
			*after = json + 2;
			st->reply.start = 255;
			return 0;
		}

		return json_lan(&(st->reply), json, after);
	}

	if(*json != '[') {
		fprintf(stderr, "Parsing puzzle step, unexpected token near %s\n", json);
		return 1;
	}

	EXPECT(json_token(json, '[', &json));
	EXPECT(json_lan(&(st->reply), json, &json));
	EXPECT(json_token(json, ',', &json));
	EXPECT(json_token(json, '{', &json));

	size_t len = 1;
	st->next = malloc(sizeof(puzzle_step_t));
	st->nextlen = 0;
	while(1) {
		if(st->nextlen == len) {
			len *= 2;
			st->next = realloc(st->next, len * sizeof(puzzle_step_t));
		}

		EXPECT(json_lan(&(st->next[st->nextlen].move), json, &json));
		EXPECT(json_token(json, ':', &json));
		EXPECT(json_puzzle_step(&(st->next[st->nextlen]), json, &json));

		++st->nextlen;
		json_whitespace(json, &json);

		if(*json == '}') break;
		if(*json != ',') {
			fprintf(stderr, "Parsing puzzle step, unexpected token near %s\n", json);
			return 1;
		}
		EXPECT(json_token(json, ',', &json));
	}

	st->next = realloc(st->next, st->nextlen * sizeof(puzzle_step_t));

	EXPECT(json_token(json, '}', &json));
	EXPECT(json_token(json, ']', &json));

	*after = json;
	return 0;
}

static int json_partial_tags(puzzle_t* p, const char* json, const char** after) {
	char tag[64];

	EXPECT(json_token(json, '[', &json));

	while(1) {
		json_whitespace(json, &json);
		if(*json == ']') break;

		EXPECT(json_str(tag, json, 64, &json));

		/* XXX: only the tags set in puzzle.c */
		if(!strncmp("Checkmate", tag, 64)) p->tags.checkmate = true;
		if(!strncmp("Draw", tag, 64)) p->tags.draw = true;
		if(!strncmp("Draw (Perpetual check)", tag, 64)) p->tags.perpetual = true;
		if(!strncmp("Draw (Threefold repetition)", tag, 64)) p->tags.threefold = true;
		if(!strncmp("Draw (Stalemate)", tag, 64)) p->tags.stalemate = true;
		if(!strncmp("Winning position", tag, 64)) p->tags.winning_position = true;

		json_whitespace(json, &json);
		if(*json == ']') break;
		if(*json == ',') {
			EXPECT(json_token(json, ',', &json));
			continue;
		}

		fprintf(stderr, "Parsing puzzle tags, unexpected token near %s\n", json);
		return 1;
	}

	EXPECT(json_token(json, ']', &json));
	*after = json;
	return 0;
}

int puzzle_load(puzzle_t* p, const char* json) {
	memset(p, 0, sizeof(puzzle_t));
	EXPECT(json_token(json, '[', &json));
	EXPECT(json_str(p->fen, json, SAFE_FEN_LENGTH, &json));
	EXPECT(json_token(json, ',', &json));
	EXPECT(json_puzzle_step(&(p->root), json, &json));
	EXPECT(json_token(json, ',', &json));
	EXPECT(json_partial_tags(p, json, &json));
	EXPECT(json_token(json, ']', &json));
	EXPECT(json_token(json, '\0', &json));
	return 0;
}
