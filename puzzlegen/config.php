<?php

assert_options(ASSERT_BAIL, true);



/* ---------- Game filtering ---------- */

/* Only consider games where both players have a rating above this
 * value */
const RATING_CUTOFF = 2000;

/* Only consider games loger than this in seconds
 * (base + 40 * increment) */
const TIME_CUTOFF = 900;



/* ---------- Puzzle filtering ---------- */

/* Don't probe for puzzles in clearly won or lost positons */
const POSITION_EVAL_CUTOFF = 500;

/* Minimum gap between best moves to consider tactics */
const BEST_MOVE_EVAL_CUTOFF = 280;

/* Skip puzzle probing for these first plies */
const MIN_PLY_CUTOFF = 4;

/* Maximum number of possible moves above EVAL_CUTOFF compared to the
 * next best to consider tactics */
const MAX_VARIATIONS = 3;

/* Only consider variations if eval difference is lower than this,
 * compared to best move */
const VARIATIONS_MAX_DIFF = 20;



/* ---------- Other settings ---------- */

/* Must be UCI compliant and have the MultiPV option */
const ENGINE = 'stockfish';

/* Will be injected as-is in the "go" UCI command */
const ENGINE_LIMITER = 'depth 16';
