/* Default options for Ormulogun puzzlegen. Can be overwritten using
 * gen-puzzles command-line args, or PFLAGS in make rules. */

static const char* uci_engine = "stockfish";

static const char* uci_engine_opts[] = {
	"Threads", "1", /* Use more than 1 thread with stockfish at your
					 * own risk, the eval fluctuates wildly and the
					 * output becomes highly nondeterministic */
	"Hash", "512",
	0
};



/* When probing for puzzles */
static const char* uci_limiter_probe = "nodes 1000000";

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

	/* XXX */
	.puzzle_threshold_absolute = 300,

	/* XXX */
	.variation_cutoff_relative = .2f,
};
