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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

int uci_create(const char* bin, uci_engine_context_t* ctx) {
	/* Blatantly inspired from pipe(3P) */
	/* XXX: can potentially deadlock because of buffering, but the engine should flush after every line */

	int fd[4]; /* parent read, child write, child read, parent write */
	int ret;

	ret = pipe(fd);
	if(ret) return ret;
	ret = pipe(&(fd[2]));
	if(ret) {
		close(fd[0]);
		close(fd[1]);
		return ret;
	}

	switch(ret = fork()) {
	case -1:
		close(fd[0]);
		close(fd[1]);
		close(fd[2]);
		close(fd[3]);
		return ret;

	case 0: /* Child */
		close(fd[0]);
		close(fd[3]);
		ret = dup2(fd[2], 0); /* stdin */
		if(ret < 0) perror("error calling dup2()");
		ret = dup2(fd[1], 1); /* stdout */
		if(ret < 0) perror("error calling dup2()");
		ret = execlp(bin, bin, 0);
		/* execlp() only returns in case of error */
		perror("error spawning engine");
		exit(1);

	default: /* Parent */
		close(fd[1]);
		close(fd[2]);
		ctx->r = fdopen(fd[0], "rb");
		ctx->w = fdopen(fd[3], "wb");
	}

	return 0;
}

void uci_quit(const uci_engine_context_t* ctx) {
	fputs("quit\n", ctx->w);
	fflush(ctx->w);
}

void uci_init(const uci_engine_context_t* ctx, const char* const* opts) {
	static char* line = 0;
	static size_t linelen;

	fputs("uci\n", ctx->w);
	for(unsigned char i = 0; opts[i]; i += 2) {
		fprintf(ctx->w, "setoption name %s value %s\n", opts[i], opts[i + 1]);
	}
	fflush(ctx->w);
	while(getline(&line, &linelen, ctx->r) != -1 && strcmp("uciok\n", line));
}

unsigned char uci_eval(const uci_engine_context_t* ctx, const char* limiter,
					   const char* lanlist, unsigned char maxlines, uci_eval_t* evals) {
	static const char* delim = " \n";
	static char* line = 0;
	static size_t linelen;

	unsigned char nlines = 0;
	char* tok;
	char* saveptr;

	fprintf(ctx->w,
			"setoption name MultiPV value %d\n"
			"position startpos moves %s\n"
			"go %s\n",
			maxlines, lanlist, limiter);
	fflush(ctx->w);

	while(getline(&line, &linelen, ctx->r) != -1) {
		tok = strtok_r(line, delim, &saveptr);
		if(!strcmp("bestmove", tok)) return nlines;
		if(strcmp("info", tok)) continue;

		bool has_mpv = false, has_pv = false, has_score = false;
		unsigned char pv;
		uci_eval_t ev;

		while((tok = strtok_r(0, delim, &saveptr))) {
			if(!strcmp("multipv", tok)) {
				tok = strtok_r(0, delim, &saveptr);
				assert(tok);
				has_mpv = true;
				pv = strtoul(tok, 0, 10) - 1;
				continue;
			}

			if(!strcmp("pv", tok)) {
				tok = strtok_r(0, delim, &saveptr);
				assert(tok);
				assert(strlen(tok) < 6);
				has_pv = true;
				strncpy(ev.bestlan, tok, 6);
				continue;
			}

			if(strcmp("score", tok)) continue;
			tok = strtok_r(0, delim, &saveptr);
			assert(tok);
			has_score = true;

			if(!strcmp("cp", tok)) {
				tok = strtok_r(0, delim, &saveptr);
				assert(tok);
				ev.type = SCORE_CP;
				ev.score = strtol(tok, 0, 10);
				continue;
			}

			if(!strcmp("mate", tok)) {
				tok = strtok_r(0, delim, &saveptr);
				assert(tok);
				ev.type = SCORE_MATE;
				ev.score = strtol(tok, 0, 10);
				continue;
			}

			assert(0);
		}

		if(has_mpv && has_pv && has_score) {
			evals[pv] = ev;
			if(pv >= nlines) nlines = pv + 1;
		}
	}

	assert(0);
	__builtin_unreachable();
}