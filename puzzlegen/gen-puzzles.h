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

#pragma once
#ifndef ORMULOGUN_GEN_PUZZLES_H
#define ORMULOGUN_GEN_PUZZLES_H

#include <stdio.h>

/* ----- uci.c ----- */

typedef struct {
	FILE* r;
	FILE* w;
} uci_engine_context_t;

typedef struct {
	enum { SCORE_CP, SCORE_MATE } type;
	int score;
	char bestlan[6];
} uci_eval_t;

int uci_create(const char*, uci_engine_context_t*);
void uci_quit(const uci_engine_context_t*);
void uci_init(const uci_engine_context_t*, const char* const*);
unsigned char uci_eval(const uci_engine_context_t*, const char*, const char*, unsigned char, uci_eval_t*);

#endif
