Generating puzzles
==================

1. Process a PGN dump into a filtered list of games (1 game per line)

   ~~~
   make INPUT=foo.pgn OUTPUT=games.jz JOBS=4
   ~~~

2. Generate puzzles

   ~~~
   # Analyze all games for all kinds of puzzles
   make puzzles-all INPUT=games.jz OUTPUT=puzzles-all.jpz # all games

   # Analyze only games 100-149
   make puzzles INPUT=games.jz OUTPUT=p START=100 END=149

   # Analyze all games for checkmate puzzles, up to 6 moves ahead
   make puzzles-all INPUT=games.jz OUTPUT=mates.jpz PFLAGS="--best-eval-cutoff-start 100000 --uci-engine-limiter-probe 'depth 12' --uci-engine-limiter 'depth 12' --max-depth 6"
   ~~~

3. Convert to JSON and update manifest

   ~~~
   make json-all
   make js-all
   ~~~
