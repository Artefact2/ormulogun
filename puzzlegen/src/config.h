/* Options for Ormulogun puzzlegen. Re-run make after changing this file. */

static const char* uci_engine = "stockfish";

static const char* uci_engine_opts[] = {
	"Threads", "8",
	"Hash", "4096",
	0
};



/* When probing for puzzles */
static const char* uci_limiter_probe = "nodes 1000000";

/* When building puzzles */
static const char* uci_limiter = "nodes 50000000";

static const puzzlegen_settings_t settings = (puzzlegen_settings_t){
	/* Abort if puzzle is longer than this number of turns */
	.max_depth = 6,

	/* Don't look for puzzles in clearly lost or won positions */
	.eval_cutoff = 500,

	/* No more than this many possible moves for the first puzzle move */
	.max_variations = 3,

	/* Consider tactics when best move is this much above last move */
	.best_eval_cutoff_start = 300,

	/* Continue tactics when best move is this much above last move */
	.best_eval_cutoff_continue = 150,

	/* Consider variations when they differ at most by this much compared to the best move */
	.variation_eval_cutoff = 50,
};