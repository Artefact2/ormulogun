/* Default options for Ormulogun puzzlegen. Can be overwritten using
 * gen-puzzles command-line args, or PFLAGS in make rules. */

static const char* uci_engine = "stockfish";

static const char* uci_engine_opts[] = {
	"Threads", "8",
	"Hash", "4096",
	0
};



/* When probing for puzzles */
static const char* uci_limiter_probe = "depth 18";

/* When building puzzles */
static const char* uci_limiter = "depth 22";

static const puzzlegen_settings_t settings = (puzzlegen_settings_t){
	/* Abort if puzzle is longer than this number of turns */
	.max_depth = 6,

	/* Look for tactics after this many plies */
	.min_ply = 8,

	/* Don't look for puzzles in clearly lost or won positions */
	.eval_cutoff = 500,

	/* No more than this many possible moves for the first puzzle move */
	.max_variations = 3,

	/* Minimum eval difference between consecutive best moves to
	 * consider a tactic */
	.puzzle_threshold_absolute = 200,

	/* Consider variations if the difference between best move and
	 * candidate move, relative to the difference between best move
	 * and threshold move, is less than this */
	.variation_cutoff_relative = .2f,
};
