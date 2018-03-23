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
#ifndef __ORMULOGUN_BOOKGEN_COMMON
#define __ORMULOGUN_BOOKGEN_COMMON

#define TURN_LIMIT 30

void gobble_whitespace(const char**);
char read_hex_digit(char);
int read_json_string(char**, size_t*, const char*, size_t);
int find_pgn_tag(const char*, const char*, char*, size_t);
const char* find_pgn_next_move(const char*, char*, size_t);
void strip_last_fields(char*, char, unsigned char);

#endif
