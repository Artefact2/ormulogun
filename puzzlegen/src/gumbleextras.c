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

bool cche_moves_through_square(const cch_move_t* m, cch_square_t sq) {
	char x1 = CCH_FILE(m->end) - CCH_FILE(m->start);
	char y1 = CCH_RANK(m->end) - CCH_RANK(m->start);
	char x2 = CCH_FILE(sq) - CCH_FILE(m->start);
	char y2 = CCH_RANK(sq) - CCH_RANK(m->start);

	if(x1 * y2 != x2 * y1) return false; /* Wrong direction */
	if(x1 * x2 < 0 || y1 * y2 < 0) return false; /* Wrong orientation */
	if(x1 * x1 + y1 * y1 < x2 * x2 + y2 * y2) return false; /* Too short */
	return true;
}
