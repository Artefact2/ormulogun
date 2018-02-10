/* Options for Ormulogun puzzlegen. Re-run make after changing this file. */

static const char* uci_engine = "stockfish";

static const char* uci_engine_opts[] = {
	"Threads", "1",
	"Hash", "20",
	0
};



/* When probing for puzzles */
static const char* uci_limiter_probe = "movetime 100";

/* When building puzzles */
static const char* uci_limiter = "movetime 500";

/* Don't look for puzzles in clearly lost or won positions */
static const int eval_cutoff = 500;

/* No more than this many possible moves for the first puzzle move */
static const unsigned char max_variations = 3;

/* Consider tactics when best move is this much above last move */
static const int best_eval_cutoff = 190;

/* Consider variations when they differ at most by this much compared to the best move */
static const int variation_eval_cutoff = 50;
